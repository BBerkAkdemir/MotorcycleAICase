// Fill out your copyright notice in the Description page of Project Settings.

#include "Pawns/AIControllers/NormalSoldierAIController.h"

#include "Pawns/AIControllers/SoldierCrowdFollowingComponent.h"

#include "Components/StateTreeComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "NavigationSystem.h"
#include "EnvironmentQuery/EnvQueryManager.h"

#include "Pawns/MotorcycleShooterPawn.h"
#include "Pawns/NormalSoldierPawn.h"
#include "Pawns/MotorcycleDriverPawn.h"

ANormalSoldierAIController::ANormalSoldierAIController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<USoldierCrowdFollowingComponent>(TEXT("PathFollowingComponent")))
{
	SetGenericTeamId(FGenericTeamId(1));

	StateTreeAIComponent = CreateDefaultSubobject<UStateTreeComponent>(TEXT("StateTreeAIComponent"));

	AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComponent"));
	SetPerceptionComponent(*AIPerceptionComponent);

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = 1500.0f;
	SightConfig->LoseSightRadius = 1800.0f;
	SightConfig->PeripheralVisionAngleDegrees = 90.0f;
	SightConfig->SetMaxAge(5.0f);
	SightConfig->AutoSuccessRangeFromLastSeenLocation = 500.0f;
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = false;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = false;

	AIPerceptionComponent->ConfigureSense(*SightConfig);
	AIPerceptionComponent->SetDominantSense(UAISense_Sight::StaticClass());
}

void ANormalSoldierAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	ControlledPawn = Cast<ANormalSoldierPawn>(InPawn);
	StateTreeAIComponent->Activate(true);
	StateTreeAIComponent->StartLogic();

	StateTreeAIComponent->SendStateTreeEvent(GameplayTag_Patrol);

	AIPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &ANormalSoldierAIController::OnTargetPerceptionUpdated);

	StartPatrol();
}

void ANormalSoldierAIController::OnUnPossess()
{
	Super::OnUnPossess();
	StateTreeAIComponent->StopLogic(FString("Cleanup"));
	StateTreeAIComponent->Deactivate();
	GetWorldTimerManager().ClearTimer(TH_SightControl);

	StopPatrol();

	AIPerceptionComponent->OnTargetPerceptionUpdated.RemoveDynamic(this, &ANormalSoldierAIController::OnTargetPerceptionUpdated);

	CacheSightControlPlayer = nullptr;
	SightActor = nullptr;
	SightActors.Empty();
	bIsRunningRetreatQuery = false;
}

ETeamAttitude::Type ANormalSoldierAIController::GetTeamAttitudeTowards(const AActor& Other) const
{
	const APawn* PawnToCheck = Cast<const APawn>(&Other);
	if (!PawnToCheck)
	{
		return ETeamAttitude::Neutral;
	}

	const IGenericTeamAgentInterface* OtherTeamAgent = Cast<const IGenericTeamAgentInterface>(PawnToCheck->GetController());
	if (OtherTeamAgent)
	{
		if (OtherTeamAgent->GetGenericTeamId() != GetGenericTeamId())
		{
			return ETeamAttitude::Hostile;
		}
		return ETeamAttitude::Friendly;
	}

	return ETeamAttitude::Neutral;
}

void ANormalSoldierAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Actor || !ControlledPawn)
	{
		return;
	}

	if (Stimulus.WasSuccessfullySensed())
	{
		bool bAlreadyTracked = false;
		for (const auto& WeakActor : SightActors)
		{
			if (WeakActor.Get() == Actor)
			{
				bAlreadyTracked = true;
				break;
			}
		}

		if (!bAlreadyTracked)
		{
			SightActors.Add(Actor);
		}

		GetWorldTimerManager().ClearTimer(TH_SightControl);
		StopPatrol();
		DistanceControl();
		GetWorldTimerManager().SetTimer(TH_SightControl, this, &ANormalSoldierAIController::DistanceControl, 1.0f, true);
	}
	else
	{
		SightActors.RemoveAll([Actor](const TWeakObjectPtr<AActor>& WeakActor)
		{
			return !WeakActor.IsValid() || WeakActor.Get() == Actor;
		});

		if (SightActors.IsEmpty())
		{
			GetWorldTimerManager().ClearTimer(TH_SightControl);
			SightActor = nullptr;
			if (ControlledPawn)
			{
				ControlledPawn->SetFireTarget(nullptr);
			}
			StateTreeAIComponent->SendStateTreeEvent(GameplayTag_Patrol);
			StartPatrol();
		}
		else
		{
			DistanceControl();
		}
	}
}

void ANormalSoldierAIController::DistanceControl()
{
	SightActors.RemoveAll([](const TWeakObjectPtr<AActor>& WeakActor)
	{
		if (!WeakActor.IsValid())
		{
			return true;
		}
		ARaidSimulationBasePawn* Pawn = Cast<ARaidSimulationBasePawn>(WeakActor.Get());
		return Pawn && Pawn->GetPoolState() != EPawnPoolState::Alive;
	});

	MostNearlyLength = TNumericLimits<float>::Max();
	CacheSightControlPlayer = nullptr;

	if (!SightActors.IsEmpty())
	{
		for (const auto& WeakActor : SightActors)
		{
			if (WeakActor.IsValid())
			{
				MostNearlyControl(WeakActor.Get());
			}
		}

		if (CacheSightControlPlayer)
		{
			if (SightActor != CacheSightControlPlayer)
			{
				SightActor = CacheSightControlPlayer;
				if (ControlledPawn)
				{
					ControlledPawn->SetFireTarget(SightActor);
				}
			}

			float DistToTarget = FVector::Dist(SightActor->GetActorLocation(), ControlledPawn->GetActorLocation());
			if (DistToTarget < RetreatDistance)
			{
				RunRetreatQuery();
			}
			else
			{
				StateTreeAIComponent->SendStateTreeEvent(GameplayTag_FireAndChase);
			}
		}
		else
		{
			GetWorldTimerManager().ClearTimer(TH_SightControl);
			SightActor = nullptr;
			if (ControlledPawn)
			{
				ControlledPawn->SetFireTarget(nullptr);
			}
			StateTreeAIComponent->SendStateTreeEvent(GameplayTag_Patrol);
			StartPatrol();
		}
	}
}

void ANormalSoldierAIController::MostNearlyControl(AActor* Actor)
{
	if (Actor && ControlledPawn)
	{
		ARaidSimulationBasePawn* TargetPawn = Cast<ARaidSimulationBasePawn>(Actor);
		if (TargetPawn && TargetPawn->GetPoolState() != EPawnPoolState::Alive)
		{
			return;
		}

		float DistSquared = FVector::DistSquared(Actor->GetActorLocation(), ControlledPawn->GetActorLocation());
		if (DistSquared < MostNearlyLength)
		{
			MostNearlyLength = DistSquared;
			CacheSightControlPlayer = Actor;
		}
	}
}

void ANormalSoldierAIController::StartPatrol()
{
	PatrolNavigate();
	GetWorldTimerManager().SetTimer(TH_PatrolNavigation, this, &ANormalSoldierAIController::PatrolNavigate, PatrolInterval, true);
}

void ANormalSoldierAIController::StopPatrol()
{
	GetWorldTimerManager().ClearTimer(TH_PatrolNavigation);
	StopMovement();
}

void ANormalSoldierAIController::PatrolNavigate()
{
	if (!ControlledPawn || SightActor)
	{
		return;
	}

	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!NavSys)
	{
		return;
	}

	FNavLocation RandomLocation;
	bool bFound = NavSys->GetRandomReachablePointInRadius(ControlledPawn->GetActorLocation(), PatrolRadius, RandomLocation);
	if (bFound)
	{
		MoveToLocation(RandomLocation.Location, 50.f);
	}
}

void ANormalSoldierAIController::RunRetreatQuery()
{
	if (!ControlledPawn || !SightActor)
	{
		return;
	}

	if (!RetreatEQS)
	{
		FVector AwayDir = (ControlledPawn->GetActorLocation() - SightActor->GetActorLocation()).GetSafeNormal();
		FVector RetreatPos = ControlledPawn->GetActorLocation() + AwayDir * RetreatMoveDistance;
		MoveToLocation(RetreatPos, 50.f);
		return;
	}

	if (bIsRunningRetreatQuery)
	{
		return;
	}

	FEnvQueryRequest Request(RetreatEQS, ControlledPawn);
	Request.Execute(EEnvQueryRunMode::SingleResult, this, &ANormalSoldierAIController::OnRetreatQueryFinished);
	bIsRunningRetreatQuery = true;
}

void ANormalSoldierAIController::OnRetreatQueryFinished(TSharedPtr<FEnvQueryResult> Result)
{
	bIsRunningRetreatQuery = false;

	if (!ControlledPawn || !SightActor)
	{
		return;
	}

	if (Result && Result->IsSuccessful())
	{
		FVector RetreatPos = Result->GetItemAsLocation(0);
		MoveToLocation(RetreatPos, 50.f);
	}
	else
	{
		FVector AwayDir = (ControlledPawn->GetActorLocation() - SightActor->GetActorLocation()).GetSafeNormal();
		FVector RetreatPos = ControlledPawn->GetActorLocation() + AwayDir * RetreatMoveDistance;
		MoveToLocation(RetreatPos, 50.f);
	}
}

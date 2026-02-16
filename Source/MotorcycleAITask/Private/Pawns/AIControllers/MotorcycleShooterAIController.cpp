// Fill out your copyright notice in the Description page of Project Settings.

#include "Pawns/AIControllers/MotorcycleShooterAIController.h"

#include "Components/StateTreeComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"

#include "Pawns/MotorcycleShooterPawn.h"
#include "Pawns/NormalSoldierPawn.h"
#include "Pawns/Components/MotorcycleShooterAnimInstance.h"

AMotorcycleShooterAIController::AMotorcycleShooterAIController()
{
	SetGenericTeamId(FGenericTeamId(0));

	StateTreeAIComponent = CreateDefaultSubobject<UStateTreeComponent>(TEXT("StateTreeAIComponent"));

	AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComponent"));
	SetPerceptionComponent(*AIPerceptionComponent);

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = 1500.0f;
	SightConfig->LoseSightRadius = 1800.0f;
	SightConfig->PeripheralVisionAngleDegrees = 360.0f;
	SightConfig->SetMaxAge(5.0f);
	SightConfig->AutoSuccessRangeFromLastSeenLocation = 500.0f;
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = false;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = false;

	AIPerceptionComponent->ConfigureSense(*SightConfig);
	AIPerceptionComponent->SetDominantSense(UAISense_Sight::StaticClass());
}

void AMotorcycleShooterAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	ControlledPawn = Cast<AMotorcycleShooterPawn>(InPawn);
	StateTreeAIComponent->Activate(true);
	StateTreeAIComponent->StartLogic();

	StateTreeAIComponent->SendStateTreeEvent(GameplayTag_Idle);

	AIPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &AMotorcycleShooterAIController::OnTargetPerceptionUpdated);
}

void AMotorcycleShooterAIController::OnUnPossess()
{
	Super::OnUnPossess();
	StateTreeAIComponent->StopLogic(FString("Cleanup"));
	StateTreeAIComponent->Deactivate();
	GetWorldTimerManager().ClearTimer(TH_SightControl);

	AIPerceptionComponent->OnTargetPerceptionUpdated.RemoveDynamic(this, &AMotorcycleShooterAIController::OnTargetPerceptionUpdated);

	CacheSightControlPlayer = nullptr;
	SightActor = nullptr;
	SightActors.Empty();
}

ETeamAttitude::Type AMotorcycleShooterAIController::GetTeamAttitudeTowards(const AActor& Other) const
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

void AMotorcycleShooterAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
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
		DistanceControl();
		GetWorldTimerManager().SetTimer(TH_SightControl, this, &AMotorcycleShooterAIController::DistanceControl, 1.0f, true);
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
			StateTreeAIComponent->SendStateTreeEvent(GameplayTag_Idle);

			if (ControlledPawn)
			{
				ControlledPawn->SetIsFiring(false);
				ControlledPawn->SetFireTarget(nullptr);
			}
		}
		else
		{
			DistanceControl();
		}
	}
}

void AMotorcycleShooterAIController::DistanceControl()
{
	SightActors.RemoveAll([](const TWeakObjectPtr<AActor>& WeakActor)
	{
		return !WeakActor.IsValid();
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
					ControlledPawn->SetIsFiring(true);
				}
			}
			StateTreeAIComponent->SendStateTreeEvent(GameplayTag_Fire);
		}
		else
		{
			GetWorldTimerManager().ClearTimer(TH_SightControl);
			SightActor = nullptr;
			StateTreeAIComponent->SendStateTreeEvent(GameplayTag_Idle);

			if (ControlledPawn)
			{
				ControlledPawn->SetIsFiring(false);
				ControlledPawn->SetFireTarget(nullptr);
			}
		}
	}
}

void AMotorcycleShooterAIController::MostNearlyControl(AActor* Actor)
{
	if (Actor && ControlledPawn)
	{
		float DistSquared = FVector::DistSquared(Actor->GetActorLocation(), ControlledPawn->GetActorLocation());
		if (DistSquared < MostNearlyLength)
		{
			MostNearlyLength = DistSquared;
			CacheSightControlPlayer = Actor;
		}
	}
}

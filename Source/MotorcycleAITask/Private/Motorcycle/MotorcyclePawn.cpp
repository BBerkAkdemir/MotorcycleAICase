// Fill out your copyright notice in the Description page of Project Settings.

#include "Motorcycle/MotorcyclePawn.h"

#pragma region UnrealComponents

#include "Components/CapsuleComponent.h"
#include "Components/AudioComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Kismet/GameplayStatics.h"

#pragma endregion

#pragma region Multiplayer Libraries

#include "Net/UnrealNetwork.h"

#pragma endregion

#pragma region InProjectIncludes

#include "Motorcycle/Components/MotorcycleMovementComponent.h"
#include "Motorcycle/MotorcycleAIController.h"
#include "Headquarters/Headquarters.h"
#include "Pawns/MotorcycleDriverPawn.h"
#include "Pawns/MotorcycleShooterPawn.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"

#pragma endregion

void AMotorcyclePawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMotorcyclePawn, AttachedDriver);
	DOREPLIFETIME(AMotorcyclePawn, AttachedShooter);
	DOREPLIFETIME(AMotorcyclePawn, MotorcycleState);
}

AMotorcyclePawn::AMotorcyclePawn()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	MaxHealth = 375.f;
	Health = MaxHealth;

	if (PerceptionStimuliSource)
	{
		PerceptionStimuliSource->bAutoRegister = false;
	}

	Capsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleRoot"));
	SetRootComponent(Capsule);
	Capsule->SetCapsuleHalfHeight(50);
	Capsule->SetCapsuleRadius(50);
	Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Capsule->SetCollisionObjectType(ECollisionChannel::ECC_Pawn);
	Capsule->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	Capsule->SetCollisionResponseToChannel(ECC_Visibility, ECollisionResponse::ECR_Ignore);
	Capsule->SetGenerateOverlapEvents(false);
	Capsule->SetCanEverAffectNavigation(false);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	Mesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->SetRelativeLocation(FVector(0, 0, -50.0f));
	Mesh->SetUsingAbsoluteRotation(true);
	Mesh->SetCanEverAffectNavigation(false);

	PawnMovement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("PawnMovementComponent"));
	PawnMovement->MaxSpeed = 600;
	PawnMovement->Acceleration = 1500;
	PawnMovement->Deceleration = 300;
	PawnMovement->TurningBoost = 0;

	MotorcycleMovementComponent = CreateDefaultSubobject<UMotorcycleMovementComponent>(TEXT("MotorcycleMovementComponent"));

	DriverSeatPoint = CreateDefaultSubobject<USceneComponent>(TEXT("DriverSeatPoint"));
	DriverSeatPoint->SetupAttachment(Mesh);
	DriverSeatPoint->SetRelativeLocation(FVector(-40.f, -45.f, 40.f));

	GunnerSeatPoint = CreateDefaultSubobject<USceneComponent>(TEXT("GunnerSeatPoint"));
	GunnerSeatPoint->SetupAttachment(Mesh);
	GunnerSeatPoint->SetRelativeLocation(FVector(-20.f, 25.f, 0.f));

	EngineAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("EngineAudioComponent"));
	EngineAudioComponent->SetupAttachment(Mesh);
	EngineAudioComponent->bAutoActivate = false;
}

void AMotorcyclePawn::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(TH_ReplicationDelay, this, &AMotorcyclePawn::PushInThePool, 2, false);
		AIControllerComponent = Cast<AMotorcycleAIController>(UAIBlueprintHelperLibrary::GetAIController(this));
	}
}

void AMotorcyclePawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AMotorcyclePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AMotorcyclePawn::OnRep_MotorcycleState()
{
}

void AMotorcyclePawn::OnSplineEndReached()
{
	if (!HasAuthority())
	{
		return;
	}

	AHeadquarters* HQ = GetOwningHeadquarters();

	if (MotorcycleState == EMotorcycleState::Approaching)
	{
		MotorcycleState = EMotorcycleState::Combat;
		if (HQ)
		{
			MotorcycleMovementComponent->SetSplinePath(HQ->GetCombatSpline(), true, true);
		}
	}
	else if (MotorcycleState == EMotorcycleState::Escaping)
	{
		SupplySoldier();
	}
}

void AMotorcyclePawn::OnCrewMemberDied(ARaidSimulationBasePawn* DeadCrew)
{
	if (!HasAuthority())
	{
		return;
	}

	if (DeadCrew == AttachedDriver)
	{
		MotorcycleState = EMotorcycleState::Stopped;
		MotorcycleMovementComponent->DeactivateMovement();
		GetWorldTimerManager().SetTimer(TH_DeathToPool, this, &ARaidSimulationBasePawn::ReturnToPoolAfterDeath, DeathToPoolDelay, false);
	}
	else if (DeadCrew == AttachedShooter)
	{
		if (MotorcycleState == EMotorcycleState::Stopped)
		{
			return;
		}

		AHeadquarters* HQ = GetOwningHeadquarters();
		if (!HQ)
		{
			return;
		}

		if (MotorcycleState == EMotorcycleState::Approaching)
		{
			MotorcycleState = EMotorcycleState::Escaping;
			MotorcycleMovementComponent->SetSplinePath(HQ->GetApproachSpline(), false, false);
		}
		else if (MotorcycleState == EMotorcycleState::Combat)
		{
			MotorcycleState = EMotorcycleState::Escaping;
			MotorcycleMovementComponent->SetSplinePath(HQ->GetRandomEscapeSpline(), true, false);
		}
	}
}

void AMotorcyclePawn::OnShooterAmmoDepleted()
{
	if (!HasAuthority())
	{
		return;
	}

	if (MotorcycleState == EMotorcycleState::Stopped)
	{
		return;
	}

	AHeadquarters* HQ = GetOwningHeadquarters();
	if (!HQ)
	{
		return;
	}

	if (MotorcycleState == EMotorcycleState::Approaching)
	{
		MotorcycleState = EMotorcycleState::Escaping;
		MotorcycleMovementComponent->SetSplinePath(HQ->GetApproachSpline(), false, false);
	}
	else if (MotorcycleState == EMotorcycleState::Combat)
	{
		MotorcycleState = EMotorcycleState::Escaping;
		MotorcycleMovementComponent->SetSplinePath(HQ->GetRandomEscapeSpline(), true, false);
	}
}

void AMotorcyclePawn::SupplySoldier()
{
	if (!HasAuthority())
	{
		return;
	}

	if (MotorcycleState != EMotorcycleState::Escaping)
	{
		return;
	}

	MotorcycleState = EMotorcycleState::Resupplying;

	if (IsValid(AttachedShooter))
	{
		if (AttachedShooter->GetPoolState() == EPawnPoolState::Dead || AttachedShooter->GetPoolState() == EPawnPoolState::InPool)
		{
			AttachedShooter->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
			AttachedShooter->CancelPendingPoolReturn();
			AttachedShooter->PullFromThePool(GunnerSeatPoint->GetComponentLocation());
			AttachedShooter->ActorAttachToComponent(GunnerSeatPoint);
			MotorcycleMovementComponent->AddIgnoredActorForTrace(AttachedShooter);
		}
		AttachedShooter->HealToMax();
		AttachedShooter->RefillAmmo();
	}

	if (IsValid(AttachedDriver))
	{
		AttachedDriver->HealToMax();
	}

	HealToMax();

	AHeadquarters* HQ = GetOwningHeadquarters();
	MotorcycleState = EMotorcycleState::Approaching;
	if (HQ)
	{
		MotorcycleMovementComponent->SetSplinePath(HQ->GetApproachSpline(), true, false);
	}
}

void AMotorcyclePawn::Internal_OnDead(FName HitBoneName, FVector ImpactNormal)
{
	Super::Internal_OnDead(HitBoneName, ImpactNormal);

	if (EngineAudioComponent)
	{
		EngineAudioComponent->Stop();
	}

	if (ExplosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, GetActorLocation());
	}

	if (HasAuthority())
	{
		if (IsValid(AIControllerComponent))
		{
			AIControllerComponent->UnPossess();
		}

		if (IsValid(AttachedDriver) && AttachedDriver->GetPoolState() != EPawnPoolState::InPool)
		{
			AttachedDriver->CancelPendingPoolReturn();
			AttachedDriver->PushInThePool();
		}
		if (IsValid(AttachedShooter) && AttachedShooter->GetPoolState() != EPawnPoolState::InPool)
		{
			AttachedShooter->CancelPendingPoolReturn();
			AttachedShooter->PushInThePool();
		}
	}

	Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Capsule->SetActive(false);
	PawnMovement->SetActive(false, true);
	MotorcycleMovementComponent->DeactivateMovement();
	MotorcycleMovementComponent->SetActive(false);
}

void AMotorcyclePawn::Internal_PushInThePool()
{
	Super::Internal_PushInThePool();

	if (EngineAudioComponent)
	{
		EngineAudioComponent->Stop();
	}

	MotorcycleState = EMotorcycleState::Idle;

	MotorcycleMovementComponent->ClearIgnoredCrewForTrace();
	MotorcycleMovementComponent->OnSplineEndReached.RemoveDynamic(this, &AMotorcyclePawn::OnSplineEndReached);

	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -50.f), FRotator(0, 0, 0));
	Mesh->SetActive(false, true);
	Mesh->SetVisibility(false);

	Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Capsule->SetActive(false);

	PawnMovement->SetActive(false, true);

	MotorcycleMovementComponent->DeactivateMovement();
	MotorcycleMovementComponent->SetActive(false, true);

	if (HasAuthority())
	{
		if (IsValid(AIControllerComponent))
		{
			AIControllerComponent->UnPossess();
		}

		if (IsValid(AttachedDriver))
		{
			AttachedDriver->CancelPendingPoolReturn();
			AttachedDriver->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
			if (AttachedDriver->GetPoolState() != EPawnPoolState::InPool)
			{
				AttachedDriver->PushInThePool();
			}
		}

		if (IsValid(AttachedShooter))
		{
			AttachedShooter->CancelPendingPoolReturn();
			AttachedShooter->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
			if (AttachedShooter->GetPoolState() != EPawnPoolState::InPool)
			{
				AttachedShooter->PushInThePool();
			}
		}

		AHeadquarters* HQ = GetOwningHeadquarters();
		if (HQ)
		{
			HQ->AddActorToPoolList(this);
		}
	}
}

void AMotorcyclePawn::Internal_PullFromThePool(FVector PullLocation)
{
	Super::Internal_PullFromThePool(PullLocation);

	if (EngineAudioComponent && EngineLoopSound)
	{
		EngineAudioComponent->SetSound(EngineLoopSound);
		EngineAudioComponent->AttenuationSettings = nullptr;
		EngineAudioComponent->bOverrideAttenuation = true;
		EngineAudioComponent->AttenuationOverrides.FalloffDistance = EngineSoundRadius;
		EngineAudioComponent->Play();
	}

	MotorcycleMovementComponent->NewRepLocation = PullLocation;
	MotorcycleMovementComponent->NewRepRotation = FRotator(0.0f, 0.0f, 0.0f);
	PawnMovement->Velocity = FVector(0.0f, 0.0f, 0.0f);
	SetActorLocationAndRotation(PullLocation, FRotator(0.0f, 0.0f, 0.0f), false, nullptr, ETeleportType::TeleportPhysics);

	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -50.f), FRotator(0, 0, 0));
	Mesh->SetActive(true, true);
	Mesh->SetVisibility(true);

	Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Capsule->SetActive(true);

	PawnMovement->SetActive(true, true);

	if (MotorcycleMovementComponent->GetIsAutoActiveMovement())
	{
		MotorcycleMovementComponent->ActivateMovement();
	}
	MotorcycleMovementComponent->SetActive(true, true);

	if (HasAuthority())
	{
		if (!AIControllerComponent)
		{
			SpawnDefaultController();
			AIControllerComponent = Cast<AMotorcycleAIController>(UAIBlueprintHelperLibrary::GetAIController(this));
		}
		else
		{
			AIControllerComponent->Possess(this);
		}

		AHeadquarters* HQ = GetOwningHeadquarters();
		if (HQ)
		{
			HQ->AddActorToAliveList(this);
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		if (!IsValid(AttachedDriver) && DriverClass)
		{
			AMotorcycleDriverPawn* Driver = GetWorld()->SpawnActor<AMotorcycleDriverPawn>(DriverClass, GetActorLocation(), FRotator::ZeroRotator, SpawnParams);
			if (Driver)
			{
				Driver->ActorAttachToComponent(DriverSeatPoint);
				AttachedDriver = Driver;
			}
		}
		else if (IsValid(AttachedDriver))
		{
			AttachedDriver->PullFromThePool(DriverSeatPoint->GetComponentLocation());
			AttachedDriver->ActorAttachToComponent(DriverSeatPoint);
		}

		if (!IsValid(AttachedShooter) && ShooterClass)
		{
			AMotorcycleShooterPawn* Shooter = GetWorld()->SpawnActor<AMotorcycleShooterPawn>(ShooterClass, GetActorLocation(), FRotator::ZeroRotator, SpawnParams);
			if (Shooter)
			{
				Shooter->ActorAttachToComponent(GunnerSeatPoint);
				AttachedShooter = Shooter;
			}
		}
		else if (IsValid(AttachedShooter))
		{
			AttachedShooter->PullFromThePool(GunnerSeatPoint->GetComponentLocation());
			AttachedShooter->ActorAttachToComponent(GunnerSeatPoint);
		}

		if (AttachedDriver)
		{
			MotorcycleMovementComponent->AddIgnoredActorForTrace(AttachedDriver);
		}
		if (AttachedShooter)
		{
			MotorcycleMovementComponent->AddIgnoredActorForTrace(AttachedShooter);
		}

		MotorcycleState = EMotorcycleState::Approaching;

		MotorcycleMovementComponent->OnSplineEndReached.AddDynamic(this, &AMotorcyclePawn::OnSplineEndReached);

		if (HQ && HQ->GetApproachSpline())
		{
			MotorcycleMovementComponent->SetSplinePath(HQ->GetApproachSpline(), true, false);
		}
	}
}

// Fill out your copyright notice in the Description page of Project Settings.

#include "Motorcycle/MotorcyclePawn.h"

#pragma region UnrealComponents

#include "Components/CapsuleComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"

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

	if (MotorcycleState == EMotorcycleState::ReturningToHQ)
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
	}
	else if (DeadCrew == AttachedShooter)
	{
		MotorcycleState = EMotorcycleState::ReturningToHQ;
		MotorcycleMovementComponent->SetSplinePath(MotorcycleMovementComponent->GetCachedSplinePath(), false);
	}
}

void AMotorcyclePawn::SupplySoldier()
{
	if (!HasAuthority())
	{
		return;
	}

	MotorcycleState = EMotorcycleState::Resupplying;

	AHeadquarters* HQ = GetOwningHeadquarters();
	if (HQ)
	{
		ARaidSimulationBasePawn* NewShooter = HQ->SpawnActorAccordingToSoldierType(ESoldierType::MotorcycleShooter);
		if (NewShooter)
		{
			NewShooter->ActorAttachToComponent(GunnerSeatPoint);
			AttachedShooter = Cast<AMotorcycleShooterPawn>(NewShooter);
			MotorcycleMovementComponent->AddIgnoredActorForTrace(NewShooter);
		}
	}

	MotorcycleState = EMotorcycleState::Combat;
	MotorcycleMovementComponent->SetSplinePath(MotorcycleMovementComponent->GetCachedSplinePath(), true);
}

void AMotorcyclePawn::Internal_OnDead(FName HitBoneName, FVector ImpactNormal)
{
	Super::Internal_OnDead(HitBoneName, ImpactNormal);

	if (HasAuthority())
	{
		if (IsValid(AIControllerComponent))
		{
			AIControllerComponent->UnPossess();
		}

		if (IsValid(AttachedDriver) && AttachedDriver->GetPoolState() == EPawnPoolState::Alive)
		{
			AttachedDriver->DamageControl(9999.f, NAME_None, FVector::ZeroVector, FVector::ZeroVector);
		}
		if (IsValid(AttachedShooter) && AttachedShooter->GetPoolState() == EPawnPoolState::Alive)
		{
			AttachedShooter->DamageControl(9999.f, NAME_None, FVector::ZeroVector, FVector::ZeroVector);
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

	AttachedDriver = nullptr;
	AttachedShooter = nullptr;
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

		AHeadquarters* HQ = GetOwningHeadquarters();
		if (HQ && HQ->GetHeadquartersParty() == EPartyType::Friendly)
		{
			HQ->AddActorToPoolList(this);
		}
	}
}

void AMotorcyclePawn::Internal_PullFromThePool(FVector PullLocation)
{
	Super::Internal_PullFromThePool(PullLocation);

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
		if (HQ && HQ->GetHeadquartersParty() == EPartyType::Friendly)
		{
			HQ->AddActorToAliveList(this);

			ARaidSimulationBasePawn* Driver = HQ->SpawnActorAccordingToSoldierType(ESoldierType::MotorcycleDriver);
			if (Driver)
			{
				Driver->ActorAttachToComponent(DriverSeatPoint);
				AttachedDriver = Cast<AMotorcycleDriverPawn>(Driver);
			}

			ARaidSimulationBasePawn* Shooter = HQ->SpawnActorAccordingToSoldierType(ESoldierType::MotorcycleShooter);
			if (Shooter)
			{
				Shooter->ActorAttachToComponent(GunnerSeatPoint);
				AttachedShooter = Cast<AMotorcycleShooterPawn>(Shooter);
			}
		}

		if (AttachedDriver)
		{
			MotorcycleMovementComponent->AddIgnoredActorForTrace(AttachedDriver);
		}
		if (AttachedShooter)
		{
			MotorcycleMovementComponent->AddIgnoredActorForTrace(AttachedShooter);
		}

		MotorcycleState = EMotorcycleState::Combat;

		MotorcycleMovementComponent->OnSplineEndReached.AddDynamic(this, &AMotorcyclePawn::OnSplineEndReached);

		if (MotorcycleMovementComponent->GetCachedSplinePath())
		{
			MotorcycleMovementComponent->SetSplinePath(MotorcycleMovementComponent->GetCachedSplinePath(), true);
		}
	}
}

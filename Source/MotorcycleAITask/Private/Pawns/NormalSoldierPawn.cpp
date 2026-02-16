// Fill out your copyright notice in the Description page of Project Settings.

#include "Pawns/NormalSoldierPawn.h"

#pragma region UnrealComponents

#include "Components/CapsuleComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Net/UnrealNetwork.h"

#pragma endregion

#pragma region InProjectIncludes

#include "Pawns/Components/SoldierMovementComponent.h"
#include "Pawns/AIControllers/NormalSoldierAIController.h"
#include "Headquarters/Headquarters.h"
#include "Pawns/Components/NormalSoldierAnimInstance.h"

#pragma endregion

void ANormalSoldierPawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANormalSoldierPawn, bIsFiring);
	DOREPLIFETIME(ANormalSoldierPawn, FireTarget);
}

ANormalSoldierPawn::ANormalSoldierPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleRoot"));
	SetRootComponent(CapsuleComponent);
	CapsuleComponent->SetCapsuleHalfHeight(50);
	CapsuleComponent->SetCapsuleRadius(50);
	CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CapsuleComponent->SetCollisionObjectType(ECollisionChannel::ECC_Pawn);
	CapsuleComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	CapsuleComponent->SetGenerateOverlapEvents(false);
	CapsuleComponent->SetCanEverAffectNavigation(false);

	SkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
	SkeletalMesh->SetupAttachment(RootComponent);
	SkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SkeletalMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	SkeletalMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	SkeletalMesh->SetGenerateOverlapEvents(false);
	SkeletalMesh->SetRelativeLocation(FVector(0, 0, -50.0f));
	SkeletalMesh->SetCanEverAffectNavigation(false);

	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(SkeletalMesh, FName("Weapon_R"));
	WeaponMesh->SetRelativeLocation(FVector(0, 0, 0));
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetGenerateOverlapEvents(false);
	WeaponMesh->SetCanEverAffectNavigation(false);

	PawnMovement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("PawnMovementComponent"));
	PawnMovement->MaxSpeed = 200;
	PawnMovement->Acceleration = 1500;
	PawnMovement->Deceleration = 300;
	PawnMovement->TurningBoost = 0;

	SoldierMovement = CreateDefaultSubobject<USoldierMovementComponent>(TEXT("SoldierMovementComponent"));
}

void ANormalSoldierPawn::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(TH_ReplicationDelay, this, &ANormalSoldierPawn::PushInThePool, 2, false);
	}
	AnimInstance = Cast<UNormalSoldierAnimInstance>(SkeletalMesh->GetAnimInstance());
}

void ANormalSoldierPawn::SetIsFiring(bool bNewFiring)
{
	if (bNewFiring && !bIsFiring)
	{
		bIsFiring = true;
		StartFiring();
	}
	else if (!bNewFiring && bIsFiring)
	{
		bIsFiring = false;
		StopFiring();
	}
}

void ANormalSoldierPawn::StartFiring()
{
	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(TH_Fire, this, &ANormalSoldierPawn::PerformFire, FireRate, true);
	}
}

void ANormalSoldierPawn::StopFiring()
{
	if (HasAuthority())
	{
		GetWorldTimerManager().ClearTimer(TH_Fire);
	}
}

void ANormalSoldierPawn::PerformFire()
{
	if (!HasAuthority() || !FireTarget || !IsValid(FireTarget) || PoolState != EPawnPoolState::Alive)
	{
		return;
	}

	FVector MuzzleLocation = WeaponMesh->GetComponentLocation();
	FVector DirectionToTarget = (FireTarget->GetActorLocation() - MuzzleLocation).GetSafeNormal();

	float SpreadRad = FMath::DegreesToRadians(FireSpreadAngle);
	DirectionToTarget = FMath::VRandCone(DirectionToTarget, SpreadRad);

	FVector TraceEnd = MuzzleLocation + DirectionToTarget * FireRange;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	FHitResult HitResult;
	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		MuzzleLocation,
		TraceEnd,
		ECC_Visibility,
		QueryParams
	);

	if (bHit)
	{
		ARaidSimulationBasePawn* HitPawn = Cast<ARaidSimulationBasePawn>(HitResult.GetActor());
		if (HitPawn)
		{
			HitPawn->DamageControl(FireDamage, HitResult.BoneName, DirectionToTarget, HitResult.ImpactNormal);
		}
		MulticastRPC_OnHitVisual(HitResult.ImpactPoint, HitResult.ImpactNormal);
	}

	MulticastRPC_OnFireVisual(MuzzleLocation, bHit ? HitResult.ImpactPoint : TraceEnd);
}

void ANormalSoldierPawn::Internal_OnDead(FName HitBoneName, FVector ImpactNormal)
{
	Super::Internal_OnDead(HitBoneName, ImpactNormal);

	StopFiring();
	bIsFiring = false;
	FireTarget = nullptr;

	if (HasAuthority() && IsValid(AIControllerComponent))
	{
		AIControllerComponent->UnPossess();
	}

	SkeletalMesh->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	SkeletalMesh->SetSimulatePhysics(true);
	SkeletalMesh->SetAllPhysicsLinearVelocity(FVector::ZeroVector);
	if (HitBoneName != NAME_None)
	{
		SkeletalMesh->AddImpulse(ImpactNormal * 4000, HitBoneName, true);
	}

	CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CapsuleComponent->SetActive(false);
	PawnMovement->SetActive(false, true);
	SoldierMovement->DeactivateMovement();
	SoldierMovement->SetActive(false);
}

void ANormalSoldierPawn::Internal_PushInThePool()
{
	Super::Internal_PushInThePool();

	StopFiring();
	bIsFiring = false;
	FireTarget = nullptr;

	SkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SkeletalMesh->SetSimulatePhysics(false);
	SkeletalMesh->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -50.f), FRotator(0, 0, 0));
	SkeletalMesh->SetActive(false, true);
	SkeletalMesh->SetVisibility(false);

	CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CapsuleComponent->SetActive(false);

	PawnMovement->SetActive(false, true);

	SoldierMovement->DeactivateMovement();
	SoldierMovement->SetActive(false, true);

	WeaponMesh->SetActive(false);
	WeaponMesh->SetVisibility(false);

	if (HasAuthority())
	{
		if (IsValid(AIControllerComponent))
		{
			AIControllerComponent->UnPossess();
		}

		AHeadquarters* HQ = GetOwningHeadquarters();
		if (HQ && HQ->GetHeadquartersParty() == EPartyType::Enemy)
		{
			HQ->AddActorToPoolList(this);
		}
	}
}

void ANormalSoldierPawn::Internal_PullFromThePool(FVector PullLocation)
{
	Super::Internal_PullFromThePool(PullLocation);

	SoldierMovement->NewRepLocation = PullLocation;
	SoldierMovement->NewRepRotation = FRotator(0.0f, 0.0f, 0.0f);
	PawnMovement->Velocity = FVector(0.0f, 0.0f, 0.0f);
	SetActorLocationAndRotation(PullLocation, FRotator(0.0f, 0.0f, 0.0f), false, nullptr, ETeleportType::TeleportPhysics);

	SkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SkeletalMesh->SetSimulatePhysics(false);
	SkeletalMesh->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -50.f), FRotator(0, -90, 0));
	SkeletalMesh->SetActive(true, true);
	SkeletalMesh->SetVisibility(true);

	CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CapsuleComponent->SetActive(true);

	PawnMovement->SetActive(true, true);

	SoldierMovement->ActivateMovement();
	SoldierMovement->SetActive(true, true);

	WeaponMesh->SetActive(true);
	WeaponMesh->SetVisibility(true);

	if (HasAuthority())
	{
		if (!AIControllerComponent)
		{
			SpawnDefaultController();
			AIControllerComponent = Cast<ANormalSoldierAIController>(UAIBlueprintHelperLibrary::GetAIController(this));
		}
		else
		{
			AIControllerComponent->Possess(this);
		}

		AHeadquarters* HQ = GetOwningHeadquarters();
		if (HQ && HQ->GetHeadquartersParty() == EPartyType::Enemy)
		{
			HQ->AddActorToAliveList(this);
		}
	}
}

// Fill out your copyright notice in the Description page of Project Settings.

#include "Pawns/MotorcycleShooterPawn.h"

#pragma region UnrealComponents

#include "Components/CapsuleComponent.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Net/UnrealNetwork.h"

#pragma endregion

#pragma region InProjectIncludes

#include "Headquarters/Headquarters.h"
#include "Pawns/Components/MotorcycleShooterAnimInstance.h"
#include "Pawns/AIControllers/MotorcycleShooterAIController.h"
#include "Motorcycle/MotorcyclePawn.h"

#pragma endregion

void AMotorcycleShooterPawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMotorcycleShooterPawn, bIsFiring);
	DOREPLIFETIME(AMotorcycleShooterPawn, FireTarget);
}

AMotorcycleShooterPawn::AMotorcycleShooterPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleRoot"));
	SetRootComponent(CapsuleComponent);
	CapsuleComponent->SetCapsuleHalfHeight(50);
	CapsuleComponent->SetCapsuleRadius(50);
	CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CapsuleComponent->SetCollisionObjectType(ECollisionChannel::ECC_Pawn);
	CapsuleComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	CapsuleComponent->SetCollisionResponseToChannel(ECC_Visibility, ECollisionResponse::ECR_Block);
	CapsuleComponent->SetCollisionResponseToChannel(ECC_Camera, ECollisionResponse::ECR_Overlap);
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
}

void AMotorcycleShooterPawn::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(TH_ReplicationDelay, this, &AMotorcycleShooterPawn::PushInThePool, 2, false);
	}
	AnimInstance = Cast<UMotorcycleShooterAnimInstance>(SkeletalMesh->GetAnimInstance());
}

void AMotorcycleShooterPawn::SetIsFiring(bool bNewFiring)
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

void AMotorcycleShooterPawn::StartFiring()
{
	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(TH_Fire, this, &AMotorcycleShooterPawn::PerformFire, FireRate, true);
	}
}

void AMotorcycleShooterPawn::StopFiring()
{
	if (HasAuthority())
	{
		GetWorldTimerManager().ClearTimer(TH_Fire);
	}
}

void AMotorcycleShooterPawn::PerformFire()
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
	AActor* ParentActor = GetAttachParentActor();
	if (ParentActor)
	{
		QueryParams.AddIgnoredActor(ParentActor);
		AMotorcyclePawn* Motorcycle = Cast<AMotorcyclePawn>(ParentActor);
		if (Motorcycle)
		{
			if (Motorcycle->GetAttachedDriver())
			{
				QueryParams.AddIgnoredActor(Motorcycle->GetAttachedDriver());
			}
		}
	}

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

void AMotorcycleShooterPawn::Internal_OnDead(FName HitBoneName, FVector ImpactNormal)
{
	Super::Internal_OnDead(HitBoneName, ImpactNormal);

	StopFiring();
	bIsFiring = false;

	if (HasAuthority())
	{
		if (IsValid(AIControllerComponent))
		{
			AIControllerComponent->UnPossess();
		}

		AMotorcyclePawn* Motorcycle = Cast<AMotorcyclePawn>(GetAttachParentActor());
		if (Motorcycle)
		{
			Motorcycle->OnCrewMemberDied(this);
		}
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
}

void AMotorcycleShooterPawn::Internal_PushInThePool()
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

	WeaponMesh->SetActive(false);
	WeaponMesh->SetVisibility(false);

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

void AMotorcycleShooterPawn::Internal_PullFromThePool(FVector PullLocation)
{
	Super::Internal_PullFromThePool(PullLocation);

	SetActorLocationAndRotation(PullLocation, FRotator(0.0f, 0.0f, 0.0f), false, nullptr, ETeleportType::TeleportPhysics);

	SkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SkeletalMesh->SetSimulatePhysics(false);
	SkeletalMesh->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -50.f), FRotator(0, -90, 0));
	SkeletalMesh->SetActive(true, true);
	SkeletalMesh->SetVisibility(true);

	CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CapsuleComponent->SetActive(true);

	WeaponMesh->SetActive(true);
	WeaponMesh->SetVisibility(true);

	if (HasAuthority())
	{
		if (!AIControllerComponent)
		{
			SpawnDefaultController();
			AIControllerComponent = Cast<AMotorcycleShooterAIController>(UAIBlueprintHelperLibrary::GetAIController(this));
			if (IsValid(AIControllerComponent))
			{
				AIControllerComponent->Possess(this);
			}
		}
		else
		{
			AIControllerComponent->Possess(this);
		}

		AHeadquarters* HQ = GetOwningHeadquarters();
		if (HQ && HQ->GetHeadquartersParty() == EPartyType::Friendly)
		{
			HQ->AddActorToAliveList(this);
		}
	}
}

void AMotorcycleShooterPawn::ActorAttachToComponent(USceneComponent* AttachedComponent)
{
	Super::ActorAttachToComponent(AttachedComponent);
}

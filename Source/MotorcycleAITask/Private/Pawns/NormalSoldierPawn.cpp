// Fill out your copyright notice in the Description page of Project Settings.

#include "Pawns/NormalSoldierPawn.h"

#pragma region UnrealComponents

#include "Components/CapsuleComponent.h"
#include "Components/AudioComponent.h"
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
	SkeletalMesh->SetRelativeLocation(FVector(0, 0, -90.0f));
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

	FireAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("FireAudioComponent"));
	FireAudioComponent->SetupAttachment(RootComponent);
	FireAudioComponent->bAutoActivate = false;
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

void ANormalSoldierPawn::OnRep_IsFiring()
{
	if (bIsFiring)
	{
		if (FireAudioComponent && FireLoopSound)
		{
			FireAudioComponent->SetSound(FireLoopSound);
			FireAudioComponent->AttenuationSettings = nullptr;
			FireAudioComponent->bOverrideAttenuation = true;
			FireAudioComponent->AttenuationOverrides.FalloffDistance = FireSoundRadius;
			FireAudioComponent->Play();
		}
	}
	else
	{
		if (FireAudioComponent)
		{
			FireAudioComponent->Stop();
		}
	}

	if (AnimInstance)
	{
		AnimInstance->SetIsFireStart(bIsFiring);
	}
}

void ANormalSoldierPawn::OnRep_FireTarget()
{
	if (AnimInstance)
	{
		AnimInstance->SetTarget(FireTarget);
	}
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

	if (AnimInstance)
	{
		AnimInstance->SetIsFireStart(bIsFiring);
	}
}

void ANormalSoldierPawn::SetFireTarget(AActor* NewTarget)
{
	FireTarget = NewTarget;
	if (AnimInstance)
	{
		AnimInstance->SetTarget(NewTarget);
	}
}

void ANormalSoldierPawn::StartFiring()
{
	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(TH_Fire, this, &ANormalSoldierPawn::PerformFire, FireRate, true);
	}

	if (FireAudioComponent && FireLoopSound)
	{
		FireAudioComponent->SetSound(FireLoopSound);
		FireAudioComponent->AttenuationSettings = nullptr;
		FireAudioComponent->bOverrideAttenuation = true;
		FireAudioComponent->AttenuationOverrides.FalloffDistance = FireSoundRadius;
		FireAudioComponent->Play();
	}
}

void ANormalSoldierPawn::StopFiring()
{
	if (HasAuthority())
	{
		GetWorldTimerManager().ClearTimer(TH_Fire);
	}

	if (FireAudioComponent)
	{
		FireAudioComponent->Stop();
	}
}

void ANormalSoldierPawn::PerformFire()
{
	if (!HasAuthority() || !FireTarget || !IsValid(FireTarget) || PoolState != EPawnPoolState::Alive)
	{
		return;
	}

	ARaidSimulationBasePawn* FireTargetPawn = Cast<ARaidSimulationBasePawn>(FireTarget);
	if (FireTargetPawn && FireTargetPawn->GetPoolState() != EPawnPoolState::Alive)
	{
		SetIsFiring(false);
		return;
	}

	FVector MuzzleLocation = CapsuleComponent->GetComponentLocation();
	FVector TargetLocation = FireTarget->GetActorLocation() + FVector(0, 0, 50);
	FVector DirectionToTarget = (TargetLocation - MuzzleLocation).GetSafeNormal();

	float SpreadRad = FMath::DegreesToRadians(FireSpreadAngle);
	DirectionToTarget = FMath::VRandCone(DirectionToTarget, SpreadRad);

	FVector TraceEnd = MuzzleLocation + DirectionToTarget * FireRange;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	AHeadquarters* HQ = GetOwningHeadquarters();
	if (HQ)
	{
		for (ARaidSimulationBasePawn* PoolPawn : HQ->GetPool())
		{
			if (IsValid(PoolPawn))
			{
				QueryParams.AddIgnoredActor(PoolPawn);
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
			if (HitPawn->DamageControl(FireDamage, HitResult.BoneName, DirectionToTarget, HitResult.ImpactNormal))
			{
				MulticastRPC_OnHitVisual(HitResult.ImpactPoint, HitResult.ImpactNormal);
			}
			MulticastRPC_OnFireVisual(MuzzleLocation, HitResult.ImpactPoint);
			return;
		}
	}

	float DistToTarget = FVector::Dist(MuzzleLocation, TargetLocation);
	float HitDist = bHit ? HitResult.Distance : FireRange;

	if (HitDist >= DistToTarget * 0.9f)
	{
		FVector ClosestPoint = FMath::ClosestPointOnSegment(TargetLocation, MuzzleLocation, TraceEnd);
		float ProximityDist = FVector::Dist(ClosestPoint, TargetLocation);

		if (ProximityDist <= 75.f)
		{
			ARaidSimulationBasePawn* TargetPawn = Cast<ARaidSimulationBasePawn>(FireTarget);
			if (TargetPawn)
			{
				if (TargetPawn->DamageControl(FireDamage, NAME_None, DirectionToTarget, -DirectionToTarget))
				{
					MulticastRPC_OnHitVisual(TargetLocation, -DirectionToTarget);
				}
				MulticastRPC_OnFireVisual(MuzzleLocation, TargetLocation);
				return;
			}
		}
	}

	MulticastRPC_OnFireVisual(MuzzleLocation, bHit ? HitResult.ImpactPoint : TraceEnd);
}

void ANormalSoldierPawn::Internal_OnDead(FName HitBoneName, FVector ImpactNormal)
{
	Super::Internal_OnDead(HitBoneName, ImpactNormal);

	if (FireAudioComponent)
	{
		FireAudioComponent->Stop();
	}

	if (HasAuthority())
	{
		StopFiring();
		bIsFiring = false;
		FireTarget = nullptr;

		if (IsValid(AIControllerComponent))
		{
			AIControllerComponent->UnPossess();
		}
	}

	if (AnimInstance)
	{
		AnimInstance->SetIsFireStart(false);
		AnimInstance->SetTarget(nullptr);
	}

	SkeletalMesh->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	SkeletalMesh->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Ignore);
	SkeletalMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
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

	if (FireAudioComponent)
	{
		FireAudioComponent->Stop();
	}

	if (HasAuthority())
	{
		StopFiring();
		bIsFiring = false;
		FireTarget = nullptr;
	}

	if (AnimInstance)
	{
		AnimInstance->SetIsFireStart(false);
		AnimInstance->SetTarget(nullptr);
	}

	SkeletalMesh->SetSimulatePhysics(false);
	SkeletalMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	SkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SkeletalMesh->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	SkeletalMesh->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -90.f), FRotator(0, 0, 0));
	SkeletalMesh->SetVisibility(false);
	SkeletalMesh->SetActive(false, false);

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

	SkeletalMesh->SetActive(true, false);
	SkeletalMesh->SetSimulatePhysics(false);
	SkeletalMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	SkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SkeletalMesh->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	SkeletalMesh->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -90.f), FRotator(0, -90, 0));
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

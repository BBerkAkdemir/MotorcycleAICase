// Fill out your copyright notice in the Description page of Project Settings.

#include "Pawns/MotorcycleDriverPawn.h"

#pragma region UnrealComponents

#include "Components/CapsuleComponent.h"

#pragma endregion

#pragma region InProjectIncludes

#include "Motorcycle/MotorcyclePawn.h"
#include "Motorcycle/Components/MotorcycleMovementComponent.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Pawns/AIControllers/MotorcycleDriverAIController.h"

#pragma endregion

AMotorcycleDriverPawn::AMotorcycleDriverPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	MaxHealth = 300.f;
	Health = MaxHealth;
	AIControllerClass = AMotorcycleDriverAIController::StaticClass();

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
	SkeletalMesh->SetRelativeRotation(FRotator(0, -90.0f, 0));
	SkeletalMesh->SetCanEverAffectNavigation(false);
}

void AMotorcycleDriverPawn::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		PoolState = EPawnPoolState::Alive;
		Health = MaxHealth;
	}
}

void AMotorcycleDriverPawn::Internal_OnDead(FName HitBoneName, FVector ImpactNormal)
{
	Super::Internal_OnDead(HitBoneName, ImpactNormal);

	if (HasAuthority())
	{
		AMotorcyclePawn* Motorcycle = Cast<AMotorcyclePawn>(GetAttachParentActor());
		if (Motorcycle)
		{
			Motorcycle->OnCrewMemberDied(this);
		}
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
}

void AMotorcycleDriverPawn::Internal_PushInThePool()
{
	Super::Internal_PushInThePool();

	SkeletalMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	SkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SkeletalMesh->SetSimulatePhysics(false);
	SkeletalMesh->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	SkeletalMesh->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -50.f), FRotator(0, -90, 0));
	SkeletalMesh->SetActive(false, true);
	SkeletalMesh->SetVisibility(false);

	CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CapsuleComponent->SetActive(false);
}

void AMotorcycleDriverPawn::Internal_PullFromThePool(FVector PullLocation)
{
	Super::Internal_PullFromThePool(PullLocation);

	SkeletalMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	SkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SkeletalMesh->SetSimulatePhysics(false);
	SkeletalMesh->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	SkeletalMesh->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -50.f), FRotator(0, -90, 0));
	SkeletalMesh->SetActive(true, true);
	SkeletalMesh->SetVisibility(true);

	CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CapsuleComponent->SetActive(true);
}

void AMotorcycleDriverPawn::ActorAttachToComponent(USceneComponent* AttachedComponent)
{
	Super::ActorAttachToComponent(AttachedComponent);
	AMotorcyclePawn* Motorcycle = Cast<AMotorcyclePawn>(AttachedComponent->GetOwner());
	if (Motorcycle)
	{
		Motorcycle->GetMotorcycleMovement()->ActivateMovement();
	}
}

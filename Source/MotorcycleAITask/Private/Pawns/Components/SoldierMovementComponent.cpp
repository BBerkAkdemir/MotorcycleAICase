// Fill out your copyright notice in the Description page of Project Settings.


#include "Pawns/Components/SoldierMovementComponent.h"

#pragma region Kismet Libraries

#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"

#pragma endregion

#pragma region Unreal Components

#include "GameFramework/FloatingPawnMovement.h"

#pragma endregion

#pragma region InProject Includes

#include "Pawns/NormalSoldierPawn.h"
#include "Structs/RaidSimulationProjectStructs.h"

#pragma endregion

USoldierMovementComponent::USoldierMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void USoldierMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerPawn = StaticCast<ANormalSoldierPawn*>(GetOwner());

	TArray<AActor*> actors;
	UGameplayStatics::GetAllActorsOfClassWithTag(this, AActor::StaticClass(), FName("Follow"), actors);
	if (!actors.IsEmpty())
	{
		FollowObject = actors[0];
	}
}

void USoldierMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (GetOwner()->HasAuthority())
	{
		MakeRotate(DeltaTime);
		MakeAddGravity(DeltaTime);
	}
	else
	{
		float Delta = FMath::Clamp(float(DeltaTime * MovementLerpMultiplier), 0.0f, 1.0f);
		GetOwner()->SetActorLocation(FMath::Lerp(GetOwner()->GetActorLocation(), NewRepLocation, Delta));
		GetOwner()->SetActorRotation(UKismetMathLibrary::RLerp(GetOwner()->GetActorRotation(), NewRepRotation, Delta, true));
		OwnerPawn->GetPawnMovement()->Velocity = FMath::Lerp(OwnerPawn->GetPawnMovement()->Velocity, NewRepVelocity, Delta);
	}
}

void USoldierMovementComponent::ApplyMovementData(const FNormalSoldierAIReplicationData& Data)
{
	FVector TempRepLocation = NewRepLocation;
	NewRepLocation = FVector(float(Data.PosX), float(Data.PosY), float(Data.PosZ));
	NewRepRotation = FRotator(0, float(Data.ActorRotation_WithYaw) * 1.41f, 0);
	NewRepVelocity = FVector(float(Data.VelX), float(Data.VelY), 0);
}

void USoldierMovementComponent::ActivateMovement()
{
	SetComponentTickEnabled(true);
}

void USoldierMovementComponent::DeactivateMovement()
{
	SetComponentTickEnabled(false);
}

void USoldierMovementComponent::MakeRotate(float DeltaTime)
{
	FVector NavVel = OwnerPawn->GetPawnMovement()->Velocity;
	if (NavVel.SquaredLength() > 20.0f)
	{
		FRotator NewRot = UKismetMathLibrary::MakeRotFromX(FVector(NavVel.X, NavVel.Y, 0));
		GetOwner()->SetActorRotation(UKismetMathLibrary::RLerp(GetOwner()->GetActorRotation(), NewRot, RotateSpeed * DeltaTime, true));
	}
}

void USoldierMovementComponent::MakeAddGravity(float DeltaTime)
{
	GetOwner()->AddActorWorldOffset(FallingVelocity * DeltaTime, true);
}


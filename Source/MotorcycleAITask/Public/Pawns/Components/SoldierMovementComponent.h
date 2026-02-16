// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SoldierMovementComponent.generated.h"

class ANormalSoldierPawn;
struct FNormalSoldierAIReplicationData;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MOTORCYCLEAITASK_API USoldierMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	USoldierMovementComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:

	FVector NewRepLocation;
	FRotator NewRepRotation;
	UPROPERTY(BlueprintReadOnly)
	FVector NewRepVelocity;

public:

	void ApplyMovementData(const FNormalSoldierAIReplicationData& Data);

protected:

	UPROPERTY(EditAnywhere)
	bool bIsAutoActiveMovement = true;

public:

	void ActivateMovement();
	void DeactivateMovement();

protected:

	TObjectPtr<ANormalSoldierPawn> OwnerPawn;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<AActor> FollowObject;

	UPROPERTY(EditAnywhere, Category = "PawnSettings | Movement")
	float RotateSpeed = 3.25f;

	UPROPERTY(EditDefaultsOnly, Category = "PawnSettings | Network")
	float MovementLerpMultiplier = 9;

	FVector FallingVelocity = FVector(0, 0, -980.0f);

	UFUNCTION()
	void MakeRotate(float DeltaTime);

	UFUNCTION()
	void MakeAddGravity(float DeltaTime);
};

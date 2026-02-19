// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MotorcycleMovementComponent.generated.h"

class AMotorcyclePawn;
class ASplinePathActor;
struct FMotorcycleAIReplicationData;

UENUM()
enum class EMotorcycleFollowMode : uint8
{
	FollowActor,
	FollowSpline
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSplineEndReached);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MOTORCYCLEAITASK_API UMotorcycleMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UMotorcycleMovementComponent();

protected:

	virtual void BeginPlay() override;

public:

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:

	FVector NewRepLocation;
	FRotator NewRepRotation;
	FRotator NewRepMeshRotation;
	FVector NewRepVelocity;

public:

	void ApplyMovementData(const FMotorcycleAIReplicationData& Data);

protected:

	TObjectPtr<AMotorcyclePawn> OwnerPawn;
	TObjectPtr<AActor> FollowObject;

	EMotorcycleFollowMode FollowMode = EMotorcycleFollowMode::FollowActor;

	UPROPERTY()
	TObjectPtr<ASplinePathActor> CachedSplinePath;

	float CurrentSplineDistance = 0.f;
	bool bSplineForward = true;
	bool bLoopSpline = false;
	FVector SplineTargetLocation;

	UPROPERTY(EditAnywhere, Category = "Spline")
	float SplineLookaheadDistance = 300.f;

	UPROPERTY(EditAnywhere, Category = "Spline")
	float SplineReachThreshold = 500.f;

	float CurrentSpeed;

	UPROPERTY(EditAnywhere)
	float MinForwardSpeed = 0.15f;

	UPROPERTY(EditAnywhere)
	float MinBackwardSpeed = -0.3f;

protected:

	UPROPERTY(EditAnywhere)
	bool bIsAutoActiveMovement = false;

public:

	bool GetIsAutoActiveMovement() { return bIsAutoActiveMovement; }
	void ActivateMovement();
	void DeactivateMovement();

	void SetSplinePath(ASplinePathActor* SplinePath, bool bForward, bool bLoop = false);
	void SetFollowTarget(AActor* Target);
	ASplinePathActor* GetCachedSplinePath() const { return CachedSplinePath; }

	UPROPERTY(BlueprintAssignable)
	FOnSplineEndReached OnSplineEndReached;

protected:

	FVector GetFollowTargetLocation() const;
	void UpdateSplineTarget();

	void CalculateSpeed(float DeltaTime);

protected:

	FCollisionQueryParams CollisionQueryParams;
	FVector ImpactLocation;
	FVector ImpactNormal;
	UPROPERTY(EditAnywhere)
	float MeshRotateSpeed = 3.0f;

protected:

	void CalculateMeshRotation(float DeltaTime);
	void DoAsyncTrace();
	void OnTraceCompleted(const FTraceHandle& Handle, FTraceDatum& Data);

public:

	void AddIgnoredActorForTrace(AActor* Actor);
	void ClearIgnoredCrewForTrace();

protected:

	UPROPERTY(EditAnywhere)
	float OwnerRotateSpeed = 50.0f;

protected:

	void CalculateOwnerRotation(float DeltaTime);

protected:

	void MoveMotorcycle();
};

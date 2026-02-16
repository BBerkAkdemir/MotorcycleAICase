// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SplinePathActor.generated.h"

class USplineComponent;

UCLASS()
class MOTORCYCLEAITASK_API ASplinePathActor : public AActor
{
	GENERATED_BODY()

public:

	ASplinePathActor();

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spline")
	TObjectPtr<USplineComponent> SplineComponent;

public:

	USplineComponent* GetSplineComponent() const { return SplineComponent; }

	FVector GetLocationAtDistance(float Distance) const;
	FVector GetDirectionAtDistance(float Distance) const;
	float GetSplineLength() const;
	float FindDistanceClosestToWorldLocation(const FVector& WorldLocation) const;
};

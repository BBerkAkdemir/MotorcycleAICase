// Fill out your copyright notice in the Description page of Project Settings.

#include "Motorcycle/SplinePathActor.h"
#include "Components/SplineComponent.h"

ASplinePathActor::ASplinePathActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponent"));
	SetRootComponent(SplineComponent);
}

FVector ASplinePathActor::GetLocationAtDistance(float Distance) const
{
	return SplineComponent->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
}

FVector ASplinePathActor::GetDirectionAtDistance(float Distance) const
{
	return SplineComponent->GetDirectionAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
}

float ASplinePathActor::GetSplineLength() const
{
	return SplineComponent->GetSplineLength();
}

float ASplinePathActor::FindDistanceClosestToWorldLocation(const FVector& WorldLocation) const
{
	float InputKey = SplineComponent->FindInputKeyClosestToWorldLocation(WorldLocation);
	return SplineComponent->GetDistanceAlongSplineAtSplineInputKey(InputKey);
}

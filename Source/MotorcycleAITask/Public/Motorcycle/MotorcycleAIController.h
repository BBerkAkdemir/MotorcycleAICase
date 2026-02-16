// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "MotorcycleAIController.generated.h"

UCLASS()
class MOTORCYCLEAITASK_API AMotorcycleAIController : public AAIController
{
	GENERATED_BODY()

public:

	AMotorcycleAIController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Retrieved owner attitude toward given Other object */
	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;
};

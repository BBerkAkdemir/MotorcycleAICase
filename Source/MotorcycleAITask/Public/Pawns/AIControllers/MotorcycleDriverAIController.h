// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "MotorcycleDriverAIController.generated.h"

UCLASS()
class MOTORCYCLEAITASK_API AMotorcycleDriverAIController : public AAIController
{
	GENERATED_BODY()

public:

	AMotorcycleDriverAIController();

	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;

};

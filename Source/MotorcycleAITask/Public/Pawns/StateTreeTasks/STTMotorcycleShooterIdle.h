// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/StateTreeTaskBlueprintBase.h"
#include "STTMotorcycleShooterIdle.generated.h"

class AMotorcycleShooterAIController;

UCLASS()
class MOTORCYCLEAITASK_API USTTMotorcycleShooterIdle : public UStateTreeTaskBlueprintBase
{
	GENERATED_BODY()

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<AMotorcycleShooterAIController> AIControllerComponent;

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) override;
};

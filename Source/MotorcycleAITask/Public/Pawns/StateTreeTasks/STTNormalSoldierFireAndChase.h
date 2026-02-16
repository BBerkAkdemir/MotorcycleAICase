// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/StateTreeTaskBlueprintBase.h"
#include "STTNormalSoldierFireAndChase.generated.h"

class ANormalSoldierAIController;

UCLASS()
class MOTORCYCLEAITASK_API USTTNormalSoldierFireAndChase : public UStateTreeTaskBlueprintBase
{
	GENERATED_BODY()

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<ANormalSoldierAIController> AIControllerComponent;

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) override;

};

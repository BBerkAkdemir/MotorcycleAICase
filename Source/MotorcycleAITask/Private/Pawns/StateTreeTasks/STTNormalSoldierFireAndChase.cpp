// Fill out your copyright notice in the Description page of Project Settings.


#include "Pawns/StateTreeTasks/STTNormalSoldierFireAndChase.h"

#include "Pawns/AIControllers/NormalSoldierAIController.h"
#include "Pawns/NormalSoldierPawn.h"
#include "Pawns/Components/NormalSoldierAnimInstance.h"

EStateTreeRunStatus USTTNormalSoldierFireAndChase::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition)
{
	if (AIControllerComponent)
	{
		if (AIControllerComponent->GetControlledPawnActor())
		{
			AIControllerComponent->GetControlledPawnActor()->SetIsFiring(true);
			return EStateTreeRunStatus::Running;
		}
	}
	return EStateTreeRunStatus::Failed;
}

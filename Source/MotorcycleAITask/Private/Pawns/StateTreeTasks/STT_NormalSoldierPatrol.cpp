// Fill out your copyright notice in the Description page of Project Settings.


#include "Pawns/StateTreeTasks/STT_NormalSoldierPatrol.h"

#include "Pawns/AIControllers/NormalSoldierAIController.h"
#include "Pawns/NormalSoldierPawn.h"
#include "Pawns/Components/NormalSoldierAnimInstance.h"

EStateTreeRunStatus USTT_NormalSoldierPatrol::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition)
{
	if (AIControllerComponent)
	{
		if (AIControllerComponent->GetControlledPawnActor())
		{
			AIControllerComponent->GetControlledPawnActor()->SetIsFiring(false);
			return EStateTreeRunStatus::Running;
		}
	}
	return EStateTreeRunStatus::Failed;
}

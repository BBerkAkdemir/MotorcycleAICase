// Fill out your copyright notice in the Description page of Project Settings.


#include "Pawns/StateTreeTasks/STTMotorcycleShooterFire.h"
#include "Pawns/AIControllers/MotorcycleShooterAIController.h"
#include "Pawns/MotorcycleShooterPawn.h"
#include "Pawns/Components/MotorcycleShooterAnimInstance.h"
#include <Kismet/KismetSystemLibrary.h>

EStateTreeRunStatus USTTMotorcycleShooterFire::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition)
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

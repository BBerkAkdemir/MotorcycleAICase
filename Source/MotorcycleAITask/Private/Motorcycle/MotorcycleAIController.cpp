// Fill out your copyright notice in the Description page of Project Settings.


#include "Motorcycle/MotorcycleAIController.h"

#include "Pawns/AIControllers/SoldierCrowdFollowingComponent.h"

AMotorcycleAIController::AMotorcycleAIController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<USoldierCrowdFollowingComponent>(TEXT("PathFollowingComponent")))
{
	SetGenericTeamId(FGenericTeamId(0));
}

ETeamAttitude::Type AMotorcycleAIController::GetTeamAttitudeTowards(const AActor& Other) const
{
	const APawn* PawnToCheck = Cast<const APawn>(&Other);
	if (!PawnToCheck)
	{
		return ETeamAttitude::Neutral;
	}

	const IGenericTeamAgentInterface* OtherTeamAgent = Cast<const IGenericTeamAgentInterface>(PawnToCheck->GetController());
	if (OtherTeamAgent)
	{
		if (OtherTeamAgent->GetGenericTeamId() != GetGenericTeamId())
		{
			return ETeamAttitude::Hostile;
		}
		return ETeamAttitude::Friendly;
	}

	return ETeamAttitude::Neutral;
}

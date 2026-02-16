// Fill out your copyright notice in the Description page of Project Settings.


#include "Pawns/AIControllers/SoldierCrowdFollowingComponent.h"

USoldierCrowdFollowingComponent::USoldierCrowdFollowingComponent()
{
	Initialize();
	SetCrowdAvoidanceQuality(ECrowdAvoidanceQuality::Low);
	SetCrowdSeparationWeight(100.0f);
	SetCrowdSeparation(true);
	SetCrowdSimulationState(ECrowdSimulationState::Enabled);
	SetCrowdCollisionQueryRange(300.0f);
	SetAvoidanceGroup(1, true);
	bEnableObstacleAvoidance = true;
	bAffectFallingVelocity = false;
	bRotateToVelocity = false;
	//SetCrowdAnticipateTurns(true);
	UpdateCrowdAgentParams();
}

int32 USoldierCrowdFollowingComponent::GetCrowdAgentAvoidanceGroup() const
{
	return 1;
}

int32 USoldierCrowdFollowingComponent::GetCrowdAgentGroupsToAvoid() const
{
	return 1;
}

int32 USoldierCrowdFollowingComponent::GetCrowdAgentGroupsToIgnore() const
{
	return 0;
}

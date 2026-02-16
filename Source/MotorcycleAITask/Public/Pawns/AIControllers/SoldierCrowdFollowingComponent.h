// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "SoldierCrowdFollowingComponent.generated.h"

/**
 * 
 */
UCLASS()
class MOTORCYCLEAITASK_API USoldierCrowdFollowingComponent : public UCrowdFollowingComponent
{
	GENERATED_BODY()

	USoldierCrowdFollowingComponent();

	virtual int32 GetCrowdAgentAvoidanceGroup() const override;
	virtual int32 GetCrowdAgentGroupsToAvoid() const override;
	virtual int32 GetCrowdAgentGroupsToIgnore() const override;
	
};

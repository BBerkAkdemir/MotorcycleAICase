// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "RaidSimulationGameState.generated.h"

UCLASS()
class MOTORCYCLEAITASK_API ARaidSimulationGameState : public AGameStateBase
{
	GENERATED_BODY()
	
public:

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UDataTable> HeadquartersDatas;

};

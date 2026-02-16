// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "GameplayTagContainer.h"
#include "Perception/AIPerceptionTypes.h"
#include "MotorcycleShooterAIController.generated.h"

class UStateTreeComponent;
class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class AMotorcycleShooterPawn;

UCLASS()
class MOTORCYCLEAITASK_API AMotorcycleShooterAIController : public AAIController
{
	GENERATED_BODY()

public:

	AMotorcycleShooterAIController();

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UStateTreeComponent> StateTreeAIComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UAIPerceptionComponent> AIPerceptionComponent;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	TObjectPtr<AMotorcycleShooterPawn> ControlledPawn;

protected:

	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	TArray<TWeakObjectPtr<AActor>> SightActors;
	TObjectPtr<AActor> SightActor;
	FTimerHandle TH_SightControl;
	float MostNearlyLength;
	TObjectPtr<AActor> CacheSightControlPlayer;

	UPROPERTY(EditAnywhere)
	FGameplayTag GameplayTag_Idle;
	UPROPERTY(EditAnywhere)
	FGameplayTag GameplayTag_Fire;

	void DistanceControl();
	void MostNearlyControl(AActor* Actor);

public:

	AMotorcycleShooterPawn* GetControlledPawnActor() { return ControlledPawn; }
	AActor* GetCurrentTarget() const { return SightActor; }

};

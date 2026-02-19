// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "GameplayTagContainer.h"
#include "Perception/AIPerceptionTypes.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "NormalSoldierAIController.generated.h"

class UStateTreeComponent;
class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class ANormalSoldierPawn;
class UEnvQuery;

UCLASS()
class MOTORCYCLEAITASK_API ANormalSoldierAIController : public AAIController
{
	GENERATED_BODY()

public:

	ANormalSoldierAIController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

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

	TObjectPtr<ANormalSoldierPawn> ControlledPawn;

protected:

	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	TArray<TWeakObjectPtr<AActor>> SightActors;
	TObjectPtr<AActor> SightActor;
	FTimerHandle TH_SightControl;
	float MostNearlyLength;
	TObjectPtr<AActor> CacheSightControlPlayer;

	UPROPERTY(EditAnywhere)
	FGameplayTag GameplayTag_Patrol;
	UPROPERTY(EditAnywhere)
	FGameplayTag GameplayTag_FireAndChase;

	void DistanceControl();
	void MostNearlyControl(AActor* Actor);

	FTimerHandle TH_PatrolNavigation;

	UFUNCTION()
	void PatrolNavigate();

	UPROPERTY(EditAnywhere, Category = "Patrol")
	float PatrolRadius = 800.f;

	UPROPERTY(EditAnywhere, Category = "Patrol")
	float PatrolInterval = 4.0f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float RetreatDistance = 600.f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float RetreatMoveDistance = 400.f;

	UPROPERTY(EditAnywhere, Category = "Combat|EQS")
	TObjectPtr<UEnvQuery> RetreatEQS;

	bool bIsRunningRetreatQuery = false;

	void RunRetreatQuery();
	void OnRetreatQueryFinished(TSharedPtr<FEnvQueryResult> Result);

public:

	ANormalSoldierPawn* GetControlledPawnActor() { return ControlledPawn; }
	AActor* GetCurrentTarget() const { return SightActor; }

	void StartPatrol();
	void StopPatrol();
};

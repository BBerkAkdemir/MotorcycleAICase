// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pawns/RaidSimulationBasePawn.h"
#include "MotorcycleDriverPawn.generated.h"

class UCapsuleComponent;

UCLASS()
class MOTORCYCLEAITASK_API AMotorcycleDriverPawn : public ARaidSimulationBasePawn
{
	GENERATED_BODY()

	AMotorcycleDriverPawn();

protected:

	virtual void BeginPlay() override;

	FTimerHandle TH_ReplicationDelay;

protected:

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UCapsuleComponent> CapsuleComponent;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> SkeletalMesh;

public:

	UCapsuleComponent* GetCapsule() { return CapsuleComponent; }
	USkeletalMeshComponent* GetSkeletalMesh() { return SkeletalMesh; }

public:

	virtual void Internal_OnDead(FName HitBoneName, FVector ImpactNormal) override;

	virtual void Internal_PushInThePool() override;

	virtual void Internal_PullFromThePool(FVector PullLocation) override;

public:

	virtual void ActorAttachToComponent(USceneComponent* AttachedComponent) override;

};

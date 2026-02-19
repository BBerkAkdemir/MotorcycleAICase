// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Enums/RaidSimulationProjectEnums.h"
#include "RaidSimulationBasePawn.generated.h"

class AHeadquarters;
class UAIPerceptionStimuliSourceComponent;
class UNiagaraSystem;

UCLASS()
class MOTORCYCLEAITASK_API ARaidSimulationBasePawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ARaidSimulationBasePawn();

protected:

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UAIPerceptionStimuliSourceComponent> PerceptionStimuliSource;

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

public:

	UPROPERTY(VisibleAnywhere, Replicated, Category = "PawnSettings | Base")
	int32 AI_ID = -1;

	UPROPERTY(EditDefaultsOnly, Category = "PawnSettings | Base")
	ESoldierType SoldierType;

protected:

	UPROPERTY(EditDefaultsOnly)
	float MaxHealth = 100;

	UPROPERTY(VisibleAnywhere, Replicated)
	float Health;

	UPROPERTY(ReplicatedUsing = OnRep_PoolState)
	EPawnPoolState PoolState = EPawnPoolState::None;

	UPROPERTY()
	TWeakObjectPtr<AHeadquarters> CachedHeadquarters;

public:

	EPawnPoolState GetPoolState() const { return PoolState; }

	AHeadquarters* GetOwningHeadquarters() const { return CachedHeadquarters.Get(); }
	void SetOwningHeadquarters(AHeadquarters* HQ);

public:

	virtual bool DamageControl(float BaseDamage, FName HitBoneName, FVector const& HitFromDirection, FVector const& ImpactNormal);

protected:

	UPROPERTY(EditDefaultsOnly, Category = "PawnSettings | Death")
	float DeathToPoolDelay = 5.0f;

	FTimerHandle TH_DeathToPool;

public:

	void ReturnToPoolAfterDeath();

	void CancelPendingPoolReturn();

	void HealToMax() { Health = MaxHealth; }

public:

	UPROPERTY(EditDefaultsOnly, Category = "VFX")
	TObjectPtr<UNiagaraSystem> HitBloodEffect;

	UPROPERTY(EditDefaultsOnly, Category = "VFX")
	TObjectPtr<UNiagaraSystem> MuzzleFlashEffect;

	virtual USceneComponent* GetMuzzleComponent() const { return nullptr; }

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastRPC_OnFireVisual(FVector MuzzleLocation, FVector HitLocation);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastRPC_OnHitVisual(FVector HitLocation, FVector HitNormal);

protected:

	UFUNCTION()
	void OnRep_PoolState();

	UFUNCTION(BlueprintCallable, NetMulticast, Reliable)
	void MulticastRPC_OnDead(FName HitBoneName, FVector ImpactNormal);

	UFUNCTION(BlueprintCallable)
	virtual void Internal_OnDead(FName HitBoneName, FVector ImpactNormal);

public:

	void PushInThePool();
	void PullFromThePool(FVector PullLocation);

protected:

	UFUNCTION(NetMulticast, Reliable)
	void MulticastRPC_PushInThePool();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastRPC_PullFromThePool(FVector PullLocation);

	UFUNCTION()
	virtual void Internal_PushInThePool();

	UFUNCTION()
	virtual void Internal_PullFromThePool(FVector PullLocation);

public:

	UFUNCTION()
	virtual void ActorAttachToComponent(USceneComponent* AttachedComponent);

};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pawns/RaidSimulationBasePawn.h"
#include "MotorcycleShooterPawn.generated.h"

class UCapsuleComponent;
class UMotorcycleShooterAnimInstance;
class AMotorcycleShooterAIController;

UCLASS()
class MOTORCYCLEAITASK_API AMotorcycleShooterPawn : public ARaidSimulationBasePawn
{
	GENERATED_BODY()

	AMotorcycleShooterPawn();

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:

	virtual void BeginPlay() override;

	FTimerHandle TH_ReplicationDelay;

protected:

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UCapsuleComponent> CapsuleComponent;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> SkeletalMesh;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UMotorcycleShooterAnimInstance> AnimInstance;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<AMotorcycleShooterAIController> AIControllerComponent;

	UPROPERTY(Replicated)
	bool bIsFiring = false;

	UPROPERTY(Replicated)
	TObjectPtr<AActor> FireTarget;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float FireRate = 0.3f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float FireDamage = 10.f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float FireRange = 1500.f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float FireSpreadAngle = 3.f;

	FTimerHandle TH_Fire;

public:

	UCapsuleComponent* GetCapsule() { return CapsuleComponent; }
	USkeletalMeshComponent* GetSkeletalMesh() { return SkeletalMesh; }
	UStaticMeshComponent* GetWeaponMesh() { return WeaponMesh; }
	UMotorcycleShooterAnimInstance* GetAnimInstance() { return AnimInstance; }

	bool GetIsFiring() const { return bIsFiring; }
	void SetIsFiring(bool bNewFiring);
	AActor* GetFireTarget() const { return FireTarget; }
	void SetFireTarget(AActor* NewTarget) { FireTarget = NewTarget; }

	void StartFiring();
	void StopFiring();
	void PerformFire();

public:

	virtual void Internal_OnDead(FName HitBoneName, FVector ImpactNormal) override;

	virtual void Internal_PushInThePool() override;

	virtual void Internal_PullFromThePool(FVector PullLocation) override;

public:

	virtual void ActorAttachToComponent(USceneComponent* AttachedComponent) override;
};

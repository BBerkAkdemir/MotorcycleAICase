// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pawns/RaidSimulationBasePawn.h"
#include "MotorcycleShooterPawn.generated.h"

class UCapsuleComponent;
class UMotorcycleShooterAnimInstance;
class AMotorcycleShooterAIController;
class UAudioComponent;

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

	UPROPERTY(ReplicatedUsing = OnRep_IsFiring)
	bool bIsFiring = false;

	UPROPERTY(ReplicatedUsing = OnRep_FireTarget)
	TObjectPtr<AActor> FireTarget;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float FireRate = 0.3f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float FireDamage = 7.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float FireRange = 1500.f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float FireSpreadAngle = 18.f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	int32 MaxAmmo = 90;

	UPROPERTY(Replicated)
	int32 CurrentAmmo;

	FTimerHandle TH_Fire;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UAudioComponent> FireAudioComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	TObjectPtr<USoundBase> FireLoopSound;

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	float FireSoundRadius = 2000.f;

public:

	UCapsuleComponent* GetCapsule() { return CapsuleComponent; }
	USkeletalMeshComponent* GetSkeletalMesh() { return SkeletalMesh; }
	UStaticMeshComponent* GetWeaponMesh() { return WeaponMesh; }
	UMotorcycleShooterAnimInstance* GetAnimInstance() { return AnimInstance; }

	bool GetIsFiring() const { return bIsFiring; }
	void SetIsFiring(bool bNewFiring);
	AActor* GetFireTarget() const { return FireTarget; }
	void SetFireTarget(AActor* NewTarget);

	UFUNCTION()
	void OnRep_IsFiring();

	UFUNCTION()
	void OnRep_FireTarget();

	void StartFiring();
	void StopFiring();
	void PerformFire();

	void RefillAmmo();
	int32 GetCurrentAmmo() const { return CurrentAmmo; }
	int32 GetMaxAmmo() const { return MaxAmmo; }

public:

	virtual USceneComponent* GetMuzzleComponent() const override { return WeaponMesh; }

	virtual void Internal_OnDead(FName HitBoneName, FVector ImpactNormal) override;

	virtual void Internal_PushInThePool() override;

	virtual void Internal_PullFromThePool(FVector PullLocation) override;

public:

	virtual void ActorAttachToComponent(USceneComponent* AttachedComponent) override;
};

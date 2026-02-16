// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pawns/RaidSimulationBasePawn.h"
#include "NormalSoldierPawn.generated.h"

class UCapsuleComponent;
class UFloatingPawnMovement;
class USoldierMovementComponent;
class ANormalSoldierAIController;
class UNormalSoldierAnimInstance;

UCLASS()
class MOTORCYCLEAITASK_API ANormalSoldierPawn : public ARaidSimulationBasePawn
{
	GENERATED_BODY()

	ANormalSoldierPawn();

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	// Called when the game starts or when spawned
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
	TObjectPtr<UFloatingPawnMovement> PawnMovement;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USoldierMovementComponent> SoldierMovement;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<ANormalSoldierAIController> AIControllerComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNormalSoldierAnimInstance> AnimInstance;

	UPROPERTY(Replicated)
	bool bIsFiring = false;

	UPROPERTY(Replicated)
	TObjectPtr<AActor> FireTarget;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float FireRate = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float FireDamage = 8.f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float FireRange = 1500.f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float FireSpreadAngle = 5.f;

	FTimerHandle TH_Fire;

public:

	UCapsuleComponent* GetCapsule() { return CapsuleComponent; }
	USkeletalMeshComponent* GetSkeletalMesh() { return SkeletalMesh; }
	UStaticMeshComponent* GetWeaponMesh() { return WeaponMesh; }
	UFloatingPawnMovement* GetPawnMovement() { return PawnMovement; }
	USoldierMovementComponent* GetSoldierMovement() { return SoldierMovement; }
	UNormalSoldierAnimInstance* GetAnimInstance() { return AnimInstance; }

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

};

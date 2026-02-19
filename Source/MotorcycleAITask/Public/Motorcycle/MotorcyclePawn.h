// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pawns/RaidSimulationBasePawn.h"
#include "Motorcycle/Interfaces/SupplyInterface.h"
#include "Enums/RaidSimulationProjectEnums.h"
#include "MotorcyclePawn.generated.h"

class UCapsuleComponent;
class UFloatingPawnMovement;
class UMotorcycleMovementComponent;
class AMotorcycleAIController;
class AMotorcycleDriverPawn;
class AMotorcycleShooterPawn;
class UAudioComponent;
class UNiagaraSystem;

UCLASS()
class MOTORCYCLEAITASK_API AMotorcyclePawn : public ARaidSimulationBasePawn, public ISupplyInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AMotorcyclePawn();

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	FTimerHandle TH_ReplicationDelay;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:

	UPROPERTY(EditAnywhere, Category = "Components")
	TObjectPtr<UCapsuleComponent> Capsule;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(EditAnywhere, Category = "Components")
	TObjectPtr<UFloatingPawnMovement> PawnMovement;

	UPROPERTY(EditAnywhere, Category = "Components")
	TObjectPtr<UMotorcycleMovementComponent> MotorcycleMovementComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<AMotorcycleAIController> AIControllerComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* DriverSeatPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* GunnerSeatPoint;

public:

	UCapsuleComponent* GetCapsule() { return Capsule; }
	UStaticMeshComponent* GetMesh() { return Mesh; }
	UFloatingPawnMovement* GetPawnMovement() { return PawnMovement; }
	UMotorcycleMovementComponent* GetMotorcycleMovement() { return MotorcycleMovementComponent; }
	USceneComponent* GetDriverSeatPoint() { return DriverSeatPoint; }
	USceneComponent* GetGunnerSeatPoint() { return GunnerSeatPoint; }

protected:

	UPROPERTY(EditDefaultsOnly, Category = "Crew")
	TSubclassOf<AMotorcycleDriverPawn> DriverClass;

	UPROPERTY(EditDefaultsOnly, Category = "Crew")
	TSubclassOf<AMotorcycleShooterPawn> ShooterClass;

	UPROPERTY(Replicated)
	TObjectPtr<AMotorcycleDriverPawn> AttachedDriver;

	UPROPERTY(Replicated)
	TObjectPtr<AMotorcycleShooterPawn> AttachedShooter;

	UPROPERTY(ReplicatedUsing = OnRep_MotorcycleState)
	EMotorcycleState MotorcycleState = EMotorcycleState::Idle;

	UFUNCTION()
	void OnRep_MotorcycleState();

public:

	AMotorcycleDriverPawn* GetAttachedDriver() const { return AttachedDriver; }
	AMotorcycleShooterPawn* GetAttachedShooter() const { return AttachedShooter; }
	EMotorcycleState GetMotorcycleState() const { return MotorcycleState; }

	void SetAttachedDriver(AMotorcycleDriverPawn* Driver) { AttachedDriver = Driver; }
	void SetAttachedShooter(AMotorcycleShooterPawn* Shooter) { AttachedShooter = Shooter; }

	void OnCrewMemberDied(ARaidSimulationBasePawn* DeadCrew);

	void OnShooterAmmoDepleted();

	UFUNCTION()
	void OnSplineEndReached();

public://Supply Interface Overrides

	virtual void SupplySoldier() override;

protected:

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UAudioComponent> EngineAudioComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	TObjectPtr<USoundBase> EngineLoopSound;

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	float EngineSoundRadius = 3000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	TObjectPtr<USoundBase> ExplosionSound;

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	float ExplosionSoundRadius = 5000.f;

	UPROPERTY(EditDefaultsOnly, Category = "VFX")
	TObjectPtr<UNiagaraSystem> ExplosionEffect;

	virtual void Internal_OnDead(FName HitBoneName, FVector ImpactNormal) override;

	virtual void Internal_PushInThePool() override;

	virtual void Internal_PullFromThePool(FVector PullLocation) override;

};

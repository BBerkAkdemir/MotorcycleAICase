// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Structs/RaidSimulationProjectStructs.h"
#include "Headquarters.generated.h"

class ARaidSimulationBasePawn;
class ASplinePathActor;

UCLASS()
class MOTORCYCLEAITASK_API AHeadquarters : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AHeadquarters();

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION()
	void PullFromPoolTryDelay();

	FTimerHandle TH_PullDelay;
	FTimerHandle TH_ContinuousSpawn;

	UFUNCTION()
	void ContinuousSpawnTick();

	int32 MaxAliveCount = 10;
	float SpawnInterval = 3.0f;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:

	UPROPERTY(EditAnywhere, Replicated)
	TObjectPtr<UStaticMeshComponent> BuildingMesh;

	UPROPERTY(EditAnywhere, Replicated)
	EPartyType HeadquartersParty;

public:

	EPartyType GetHeadquartersParty() const { return HeadquartersParty; }

protected:

	UPROPERTY(VisibleAnywhere, Replicated)
	TArray<TObjectPtr<ARaidSimulationBasePawn>> Pool;

	UPROPERTY(VisibleAnywhere)
	TArray<int> SoldiersOnAliveIndexes;

public:

	const TArray<TObjectPtr<ARaidSimulationBasePawn>>& GetPool() const { return Pool; }

	void AddActorToPoolList(ARaidSimulationBasePawn* Actor);
	void AddActorToAliveList(ARaidSimulationBasePawn* Actor);

protected:

	UPROPERTY(EditInstanceOnly, Category = "Splines")
	TObjectPtr<ASplinePathActor> ApproachSpline;

	UPROPERTY(EditInstanceOnly, Category = "Splines")
	TObjectPtr<ASplinePathActor> CombatSpline;

	UPROPERTY(EditInstanceOnly, Category = "Splines")
	TArray<TObjectPtr<ASplinePathActor>> EscapeSplines;

public:

	ASplinePathActor* GetApproachSpline() const { return ApproachSpline; }
	ASplinePathActor* GetCombatSpline() const { return CombatSpline; }
	ASplinePathActor* GetRandomEscapeSpline() const;
	const TArray<TObjectPtr<ASplinePathActor>>& GetEscapeSplines() const { return EscapeSplines; }

	UFUNCTION()
	ARaidSimulationBasePawn* SpawnActorAccordingToSoldierType(ESoldierType GetSoldierType);

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	uint8 MaxNumOfReplicateAtATime = 30;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	uint8 CurrentReplicationOrder = 0;

	void TickFrameReplication();

	void CreateMotorcycleReplicationData(const TArray<int>& OptimizeAIsToUpdate);
	void SerializeMotorcycleAIReplicationArray(const TArray<FMotorcycleAIReplicationData>& InArray, TArray<uint8>& OutBytes);
	void DeserializeMotorcycleAIReplicationArray(const TArray<uint8>& InBytes, TArray<FMotorcycleAIReplicationData>& OutArray);
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastRPC_ReceiveMotorcycleReplicationData(const TArray<uint8>& CompressedData);
	UFUNCTION()
	void ReceiveMotorcycleAIData(const TArray<FMotorcycleAIReplicationData>& AIData);


	void CreateSoldierReplicationData(const TArray<int>& OptimizeAIsToUpdate);
	void SerializeSoldierAIReplicationArray(const TArray<FNormalSoldierAIReplicationData>& InArray, TArray<uint8>& OutBytes);
	void DeserializeSoldierAIReplicationArray(const TArray<uint8>& InBytes, TArray<FNormalSoldierAIReplicationData>& OutArray);
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastRPC_ReceiveSoldierReplicationData(const TArray<uint8>& CompressedData);
	UFUNCTION()
	void ReceiveSoldierAIData(const TArray<FNormalSoldierAIReplicationData>& AIData);

};

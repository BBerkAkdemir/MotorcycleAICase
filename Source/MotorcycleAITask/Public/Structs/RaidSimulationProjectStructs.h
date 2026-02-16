// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Enums/RaidSimulationProjectEnums.h"
#include "RaidSimulationProjectStructs.generated.h"

USTRUCT(BlueprintType)
struct FHeadquartersPluginData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<USceneComponent> PluginClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UPrimaryDataAsset> PluginDataAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTransform PluginRelativeSpawnTransform;
};

class ARaidSimulationBasePawn;

USTRUCT(BlueprintType)
struct FSoldierPoolData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ESoldierType SoldierType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<ARaidSimulationBasePawn> SoldierClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int PoolCount;
};

USTRUCT(BlueprintType)
struct FHeadquartersData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EPartyType PartyType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UStaticMesh> HeadquartersBuildingMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int MaxSpawnedSoldierCount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SoldierSpawnRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FSoldierPoolData> SoldierPoolDatas;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FHeadquartersPluginData> HeadquartersPlugins;
};

USTRUCT(BlueprintType)
struct FMotorcycleAIReplicationData
{
	GENERATED_BODY()

public:

	UPROPERTY()
	uint16 AI_ID{ 0 };

	UPROPERTY()
	int16 PosX{ 0 };
	UPROPERTY()
	int16 PosY{ 0 };
	UPROPERTY()
	int16 PosZ{ 0 };

	UPROPERTY()
	uint8 ActorRotation_WithYaw{ 0 };

	UPROPERTY()
	uint8 MeshRotation_WithPitch{ 0 };

	UPROPERTY()
	uint8 MeshRotation_WithYaw{ 0 };

	UPROPERTY()
	uint8 MeshRotation_WithRoll{ 0 };

	friend FArchive& operator<<(FArchive& Ar, FMotorcycleAIReplicationData& Data)
	{
		Ar << Data.AI_ID;
		Ar << Data.PosX;
		Ar << Data.PosY;
		Ar << Data.PosZ;
		Ar << Data.ActorRotation_WithYaw;
		Ar << Data.MeshRotation_WithPitch;
		Ar << Data.MeshRotation_WithYaw;
		Ar << Data.MeshRotation_WithRoll;
		return Ar;
	}
};

USTRUCT(BlueprintType)
struct FNormalSoldierAIReplicationData
{
	GENERATED_BODY()

public:

	UPROPERTY()
	uint16 AI_ID{ 0 };

	UPROPERTY()
	int16 PosX{ 0 };
	UPROPERTY()
	int16 PosY{ 0 };
	UPROPERTY()
	int16 PosZ{ 0 };

	UPROPERTY()
	int8 VelX{ 0 };
	UPROPERTY()
	int8 VelY{ 0 };

	UPROPERTY()
	uint8 ActorRotation_WithYaw{ 0 };

	friend FArchive& operator<<(FArchive& Ar, FNormalSoldierAIReplicationData& Data)
	{
		Ar << Data.AI_ID;
		Ar << Data.PosX;
		Ar << Data.PosY;
		Ar << Data.PosZ;
		Ar << Data.VelX;
		Ar << Data.VelY;
		Ar << Data.ActorRotation_WithYaw;
		return Ar;
	}
};
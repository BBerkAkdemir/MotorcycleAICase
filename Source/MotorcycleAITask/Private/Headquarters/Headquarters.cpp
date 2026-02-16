// Fill out your copyright notice in the Description page of Project Settings.

#include "Headquarters/Headquarters.h"

#pragma region KismetLibraries

#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

#pragma endregion

#pragma region InProjectIncludes

#include "GameStates/RaidSimulationGameState.h"
#include "Enums/RaidSimulationProjectEnums.h"
#include "Structs/RaidSimulationProjectStructs.h"
#include "Headquarters/Plugins/HeadquartersPluginBase.h"
#include "Pawns/RaidSimulationBasePawn.h"
#include "Motorcycle/MotorcyclePawn.h"
#include "Motorcycle/Components/MotorcycleMovementComponent.h"
#include "Pawns/NormalSoldierPawn.h"
#include "Pawns/Components/SoldierMovementComponent.h"

#pragma endregion

#pragma region Unreal Components

#include "GameFramework/PawnMovementComponent.h"

#pragma endregion

#pragma region Multiplayer Libraries

#include "Net/UnrealNetwork.h"
#include "Serialization/BufferArchive.h"
#include "Serialization/MemoryReader.h"

#pragma endregion

AHeadquarters::AHeadquarters()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bAlwaysRelevant = true;

	BuildingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BuildingMesh"));
	SetRootComponent(BuildingMesh);
}

void AHeadquarters::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AHeadquarters, BuildingMesh);
	DOREPLIFETIME(AHeadquarters, HeadquartersParty);
	DOREPLIFETIME(AHeadquarters, Pool);
}

void AHeadquarters::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		FText PartyTypeString = UEnum::GetDisplayValueAsText(HeadquartersParty);
		FHeadquartersData* Data = Cast<ARaidSimulationGameState>(GetWorld()->GetGameState())->HeadquartersDatas->FindRow<FHeadquartersData>(FName(*PartyTypeString.ToString()), FString());

		if (!Data)
		{
			return;
		}

		BuildingMesh->SetStaticMesh(Data->HeadquartersBuildingMesh);
		MaxAliveCount = Data->MaxSpawnedSoldierCount;
		SpawnInterval = Data->SoldierSpawnRate;

		for (int i = 0; i < Data->HeadquartersPlugins.Num(); i++)
		{
			FString ComponentName = Data->HeadquartersPlugins[i].PluginClass->GetName();
			ComponentName.Append("_Spawned");

			TObjectPtr<UHeadquartersPluginBase> CreatedObject = NewObject<UHeadquartersPluginBase>(this, Data->HeadquartersPlugins[i].PluginClass, *ComponentName, EObjectFlags::RF_Transient, nullptr, true);
			CreatedObject->InitializeArea(Data->HeadquartersPlugins[i].PluginDataAsset);
			CreatedObject->RegisterComponent();
		}

		int PoolSize = 0;
		for (int i = 0; i < Data->SoldierPoolDatas.Num(); i++)
		{
			PoolSize += Data->SoldierPoolDatas[i].PoolCount;
		}
		Pool.SetNum(PoolSize);

		int CurrentIndex = 0;
		for (int i = 0; i < Data->SoldierPoolDatas.Num(); i++)
		{
			for (int j = 0; j < Data->SoldierPoolDatas[i].PoolCount; j++)
			{
				FActorSpawnParameters SpawnParameters;
				SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
				Pool[CurrentIndex] = GetWorld()->SpawnActor<ARaidSimulationBasePawn>(Data->SoldierPoolDatas[i].SoldierClass, GetActorLocation() + FVector(UKismetMathLibrary::RandomFloatInRange(-200, 200), UKismetMathLibrary::RandomFloatInRange(-200, 200), 50), FRotator::ZeroRotator, SpawnParameters);
				Pool[CurrentIndex]->AI_ID = CurrentIndex;
				Pool[CurrentIndex]->SetOwningHeadquarters(this);
				CurrentIndex++;
			}
		}

		GetWorldTimerManager().SetTimer(TH_PullDelay, this, &AHeadquarters::PullFromPoolTryDelay, 3, false);
	}
}

void AHeadquarters::PullFromPoolTryDelay()
{
	SpawnActorAccordingToSoldierType(ESoldierType::Motorcycle);
	SpawnActorAccordingToSoldierType(ESoldierType::NormalShooter);

	GetWorldTimerManager().SetTimer(TH_ContinuousSpawn, this, &AHeadquarters::ContinuousSpawnTick, SpawnInterval, true);
}

void AHeadquarters::ContinuousSpawnTick()
{
	for (int32 i = 0; i < Pool.Num(); i++)
	{
		if (IsValid(Pool[i]) && Pool[i]->GetPoolState() == EPawnPoolState::Dead)
		{
			Pool[i]->PushInThePool();
		}
	}

	for (int32 i = SoldiersOnAliveIndexes.Num() - 1; i >= 0; i--)
	{
		int32 AliveIndex = SoldiersOnAliveIndexes[i];
		if (!Pool.IsValidIndex(AliveIndex) || !IsValid(Pool[AliveIndex]) || Pool[AliveIndex]->GetPoolState() != EPawnPoolState::Alive)
		{
			SoldiersOnAliveIndexes.RemoveAtSwap(i);
		}
	}

	if (SoldiersOnAliveIndexes.Num() >= MaxAliveCount)
	{
		return;
	}

	SpawnActorAccordingToSoldierType(ESoldierType::Motorcycle);

	if (SoldiersOnAliveIndexes.Num() < MaxAliveCount)
	{
		SpawnActorAccordingToSoldierType(ESoldierType::NormalShooter);
	}
}

void AHeadquarters::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (HasAuthority())
	{
		TickFrameReplication();
	}
}

void AHeadquarters::AddActorToPoolList(ARaidSimulationBasePawn* Actor)
{
	int32 RemoveIndex = SoldiersOnAliveIndexes.Find(Actor->AI_ID);
	if (RemoveIndex != INDEX_NONE)
	{
		SoldiersOnAliveIndexes.RemoveAtSwap(RemoveIndex);
	}
}

void AHeadquarters::AddActorToAliveList(ARaidSimulationBasePawn* Actor)
{
	SoldiersOnAliveIndexes.AddUnique(Actor->AI_ID);
}

ARaidSimulationBasePawn* AHeadquarters::SpawnActorAccordingToSoldierType(ESoldierType GetSoldierType)
{
	for (int32 i = 0; i < Pool.Num(); i++)
	{
		if (!IsValid(Pool[i]))
		{
			continue;
		}

		if (Pool[i]->SoldierType == GetSoldierType && Pool[i]->GetPoolState() == EPawnPoolState::InPool)
		{
			Pool[i]->PullFromThePool(GetActorLocation() + FVector(250, UKismetMathLibrary::RandomFloatInRange(-200, 200), 100));
			return Pool[i];
		}
	}

	return nullptr;
}

void AHeadquarters::TickFrameReplication()
{
	if (SoldiersOnAliveIndexes.IsEmpty())
	{
		return;
	}

	const int32 MaxIndex = SoldiersOnAliveIndexes.Num();
	int32 StartIndex = MaxNumOfReplicateAtATime * CurrentReplicationOrder;
	const int32 EndIndex = FMath::Min(StartIndex + MaxNumOfReplicateAtATime, MaxIndex);

	TArray<int> MotorcycleAIs;
	TArray<int> SoldierAIs;

	for (int32 i = StartIndex; i < EndIndex; i++)
	{
		int32 AliveIndex = SoldiersOnAliveIndexes[i];
		if (!Pool.IsValidIndex(AliveIndex) || !IsValid(Pool[AliveIndex]))
		{
			continue;
		}

		if (Pool[AliveIndex]->SoldierType == ESoldierType::Motorcycle)
		{
			MotorcycleAIs.Add(AliveIndex);
		}
		else if (Pool[AliveIndex]->SoldierType == ESoldierType::NormalShooter)
		{
			SoldierAIs.Add(AliveIndex);
		}
	}

	if (!MotorcycleAIs.IsEmpty())
	{
		CreateMotorcycleReplicationData(MotorcycleAIs);
	}
	if (!SoldierAIs.IsEmpty())
	{
		CreateSoldierReplicationData(SoldierAIs);
	}

	if (EndIndex >= MaxIndex)
	{
		CurrentReplicationOrder = 0;
	}
	else
	{
		++CurrentReplicationOrder;
	}
}

void AHeadquarters::CreateMotorcycleReplicationData(const TArray<int>& OptimizeAIsToUpdate)
{
	TArray<FMotorcycleAIReplicationData> DataToSend;

	for (int32 i = 0; i < OptimizeAIsToUpdate.Num(); i++)
	{
		auto AI = Pool[OptimizeAIsToUpdate[i]];
		if (!IsValid(AI))
		{
			continue;
		}
		FMotorcycleAIReplicationData RepData;
		RepData.AI_ID = OptimizeAIsToUpdate[i];
		RepData.PosX = AI->GetActorLocation().X;
		RepData.PosY = AI->GetActorLocation().Y;
		RepData.PosZ = AI->GetActorLocation().Z;
		RepData.ActorRotation_WithYaw = FMath::RoundToInt(AI->GetActorRotation().Yaw / 1.41f);
		RepData.MeshRotation_WithPitch = FMath::RoundToInt(AI->GetActorRotation().Pitch / 1.41f);
		RepData.MeshRotation_WithYaw = FMath::RoundToInt(AI->GetActorRotation().Yaw / 1.41f);
		RepData.MeshRotation_WithRoll = FMath::RoundToInt(AI->GetActorRotation().Roll / 1.41f);

		DataToSend.Add(RepData);
	}

	if (DataToSend.IsEmpty())
	{
		return;
	}

	TArray<uint8> UncompressedBytes;
	SerializeMotorcycleAIReplicationArray(DataToSend, UncompressedBytes);

	const int32 UncompressedSize = UncompressedBytes.Num();
	int32 CompressedBufferSize = FCompression::CompressMemoryBound(NAME_Zlib, UncompressedSize);
	TArray<uint8> OutCompressedData;
	OutCompressedData.SetNumUninitialized(sizeof(int32) + CompressedBufferSize);

	FMemory::Memcpy(OutCompressedData.GetData(), &UncompressedSize, sizeof(int32));

	bool bSuccess = FCompression::CompressMemory(
		NAME_Zlib,
		OutCompressedData.GetData() + sizeof(int32),
		CompressedBufferSize,
		UncompressedBytes.GetData(),
		UncompressedSize
	);

	if (bSuccess)
	{
		// Use actual compressed size, not CompressMemoryBound
		OutCompressedData.SetNum(sizeof(int32) + CompressedBufferSize);
		MulticastRPC_ReceiveMotorcycleReplicationData(OutCompressedData);
	}
}

void AHeadquarters::SerializeMotorcycleAIReplicationArray(const TArray<FMotorcycleAIReplicationData>& InArray, TArray<uint8>& OutBytes)
{
	const int32 NumElements = InArray.Num();
	OutBytes.SetNumUninitialized(NumElements * sizeof(FMotorcycleAIReplicationData));
	FMemory::Memcpy(OutBytes.GetData(), InArray.GetData(), OutBytes.Num());
}

void AHeadquarters::DeserializeMotorcycleAIReplicationArray(const TArray<uint8>& InBytes, TArray<FMotorcycleAIReplicationData>& OutArray)
{
	const int32 NumElements = InBytes.Num() / sizeof(FMotorcycleAIReplicationData);
	OutArray.SetNumUninitialized(NumElements);
	FMemory::Memcpy(OutArray.GetData(), InBytes.GetData(), InBytes.Num());
}

void AHeadquarters::MulticastRPC_ReceiveMotorcycleReplicationData_Implementation(const TArray<uint8>& CompressedData)
{
	if (HasAuthority())
	{
		return;
	}
	if (CompressedData.Num() < sizeof(int32))
	{
		UE_LOG(LogTemp, Error, TEXT("Compressed data too small"));
		return;
	}

	int32 UncompressedSize = 0;
	FMemory::Memcpy(&UncompressedSize, CompressedData.GetData(), sizeof(int32));

	TArray<uint8> UncompressedBytes;
	UncompressedBytes.SetNumUninitialized(UncompressedSize);

	const uint8* CompressedDataStart = CompressedData.GetData() + sizeof(int32);
	const int32 CompressedSize = CompressedData.Num() - sizeof(int32);

	bool bSuccess = FCompression::UncompressMemory(
		NAME_Zlib,
		UncompressedBytes.GetData(),
		UncompressedSize,
		CompressedDataStart,
		CompressedSize
	);

	if (!bSuccess)
	{
		UE_LOG(LogTemp, Error, TEXT("Decompression failed!"));
		return;
	}

	TArray<FMotorcycleAIReplicationData> OutArray;
	DeserializeMotorcycleAIReplicationArray(UncompressedBytes, OutArray);
	ReceiveMotorcycleAIData(OutArray);
}

void AHeadquarters::ReceiveMotorcycleAIData(const TArray<FMotorcycleAIReplicationData>& AIData)
{
	for (int i = 0; i < AIData.Num(); i++)
	{
		if (!Pool.IsValidIndex(AIData[i].AI_ID))
		{
			continue;
		}
		AMotorcyclePawn* Motorcycle = Cast<AMotorcyclePawn>(Pool[AIData[i].AI_ID]);
		if (Motorcycle && Motorcycle->GetMotorcycleMovement())
		{
			Motorcycle->GetMotorcycleMovement()->ApplyMovementData(AIData[i]);
		}
	}
}

void AHeadquarters::CreateSoldierReplicationData(const TArray<int>& OptimizeAIsToUpdate)
{
	TArray<FNormalSoldierAIReplicationData> DataToSend;

	for (int32 i = 0; i < OptimizeAIsToUpdate.Num(); i++)
	{
		auto AI = Pool[OptimizeAIsToUpdate[i]];
		if (!IsValid(AI))
		{
			continue;
		}
		FNormalSoldierAIReplicationData RepData;
		RepData.AI_ID = OptimizeAIsToUpdate[i];
		RepData.PosX = AI->GetActorLocation().X;
		RepData.PosY = AI->GetActorLocation().Y;
		RepData.PosZ = AI->GetActorLocation().Z;
		if (AI->GetMovementComponent())
		{
			RepData.VelX = AI->GetMovementComponent()->Velocity.X;
			RepData.VelY = AI->GetMovementComponent()->Velocity.Y;
		}
		RepData.ActorRotation_WithYaw = FMath::RoundToInt(AI->GetActorRotation().Yaw / 1.41f);

		DataToSend.Add(RepData);
	}

	if (DataToSend.IsEmpty())
	{
		return;
	}

	TArray<uint8> UncompressedBytes;
	SerializeSoldierAIReplicationArray(DataToSend, UncompressedBytes);

	const int32 UncompressedSize = UncompressedBytes.Num();
	int32 CompressedBufferSize = FCompression::CompressMemoryBound(NAME_Zlib, UncompressedSize);
	TArray<uint8> OutCompressedData;
	OutCompressedData.SetNumUninitialized(sizeof(int32) + CompressedBufferSize);

	FMemory::Memcpy(OutCompressedData.GetData(), &UncompressedSize, sizeof(int32));

	bool bSuccess = FCompression::CompressMemory(
		NAME_Zlib,
		OutCompressedData.GetData() + sizeof(int32),
		CompressedBufferSize,
		UncompressedBytes.GetData(),
		UncompressedSize
	);

	if (bSuccess)
	{
		// Use actual compressed size, not CompressMemoryBound
		OutCompressedData.SetNum(sizeof(int32) + CompressedBufferSize);
		MulticastRPC_ReceiveSoldierReplicationData(OutCompressedData);
	}
}

void AHeadquarters::SerializeSoldierAIReplicationArray(const TArray<FNormalSoldierAIReplicationData>& InArray, TArray<uint8>& OutBytes)
{
	const int32 NumElements = InArray.Num();
	OutBytes.SetNumUninitialized(NumElements * sizeof(FNormalSoldierAIReplicationData));
	FMemory::Memcpy(OutBytes.GetData(), InArray.GetData(), OutBytes.Num());
}

void AHeadquarters::DeserializeSoldierAIReplicationArray(const TArray<uint8>& InBytes, TArray<FNormalSoldierAIReplicationData>& OutArray)
{
	const int32 NumElements = InBytes.Num() / sizeof(FNormalSoldierAIReplicationData);
	OutArray.SetNumUninitialized(NumElements);
	FMemory::Memcpy(OutArray.GetData(), InBytes.GetData(), InBytes.Num());
}

void AHeadquarters::MulticastRPC_ReceiveSoldierReplicationData_Implementation(const TArray<uint8>& CompressedData)
{
	if (HasAuthority())
	{
		return;
	}
	if (CompressedData.Num() < sizeof(int32))
	{
		UE_LOG(LogTemp, Error, TEXT("Compressed data too small"));
		return;
	}

	int32 UncompressedSize = 0;
	FMemory::Memcpy(&UncompressedSize, CompressedData.GetData(), sizeof(int32));

	TArray<uint8> UncompressedBytes;
	UncompressedBytes.SetNumUninitialized(UncompressedSize);

	const uint8* CompressedDataStart = CompressedData.GetData() + sizeof(int32);
	const int32 CompressedSize = CompressedData.Num() - sizeof(int32);

	bool bSuccess = FCompression::UncompressMemory(
		NAME_Zlib,
		UncompressedBytes.GetData(),
		UncompressedSize,
		CompressedDataStart,
		CompressedSize
	);

	if (!bSuccess)
	{
		UE_LOG(LogTemp, Error, TEXT("Decompression failed!"));
		return;
	}

	TArray<FNormalSoldierAIReplicationData> OutArray;
	DeserializeSoldierAIReplicationArray(UncompressedBytes, OutArray);
	ReceiveSoldierAIData(OutArray);
}

void AHeadquarters::ReceiveSoldierAIData(const TArray<FNormalSoldierAIReplicationData>& AIData)
{
	for (int i = 0; i < AIData.Num(); i++)
	{
		if (!Pool.IsValidIndex(AIData[i].AI_ID))
		{
			continue;
		}
		ANormalSoldierPawn* NormalSoldier = Cast<ANormalSoldierPawn>(Pool[AIData[i].AI_ID]);
		if (NormalSoldier && NormalSoldier->GetSoldierMovement())
		{
			NormalSoldier->GetSoldierMovement()->ApplyMovementData(AIData[i]);
		}
	}
}

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RaidSimulationProjectEnums.generated.h"

UENUM()
enum class EPartyType : uint8
{
	Friendly,
	Enemy
};

UENUM()
enum class ESoldierType : uint8
{
	Motorcycle,
	MotorcycleDriver,
	MotorcycleShooter,
	NormalShooter
};

UENUM()
enum class EPawnPoolState : uint8
{
	None,
	InPool,
	Alive,
	Dead
};

UENUM(BlueprintType)
enum class EMotorcycleState : uint8
{
	Idle,
	Approaching,
	Combat,
	Escaping,
	Resupplying,
	Stopped
};


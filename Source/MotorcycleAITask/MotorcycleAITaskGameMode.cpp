// Copyright Epic Games, Inc. All Rights Reserved.

#include "MotorcycleAITaskGameMode.h"
#include "MotorcycleAITaskCharacter.h"
#include "UObject/ConstructorHelpers.h"

AMotorcycleAITaskGameMode::AMotorcycleAITaskGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}

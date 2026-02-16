// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SupplyAreaDA.generated.h"

UCLASS()
class MOTORCYCLEAITASK_API USupplyAreaDA : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Supply Area Data")
	TObjectPtr<UStaticMesh> NewAreaMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Supply Area Data")
	FTransform AreaTransform;
};

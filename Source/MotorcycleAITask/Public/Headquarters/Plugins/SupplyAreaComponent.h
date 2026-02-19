// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Headquarters/Plugins/HeadquartersPluginBase.h"
#include "SupplyAreaComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MOTORCYCLEAITASK_API USupplyAreaComponent : public UHeadquartersPluginBase
{
	GENERATED_BODY()

public:

	USupplyAreaComponent();

protected:
	virtual void BeginPlay() override;

public:

	virtual void InitializeArea(UPrimaryDataAsset* PluginData) override;

protected:

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> AreaMesh;

protected:

	// Overlap Fonksiyonlar�
	UFUNCTION()
	void OnMeshBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

};

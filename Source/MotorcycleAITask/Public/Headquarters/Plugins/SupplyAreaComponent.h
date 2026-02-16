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

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

public:

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:

	virtual void InitializeArea(UPrimaryDataAsset* PluginData) override;

protected:

	UPROPERTY(EditAnywhere, Replicated)
	TObjectPtr<UStaticMeshComponent> AreaMesh;

protected:

	// Overlap Fonksiyonlarý
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

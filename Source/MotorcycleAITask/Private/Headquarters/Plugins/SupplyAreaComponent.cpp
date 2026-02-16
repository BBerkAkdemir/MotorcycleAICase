// Fill out your copyright notice in the Description page of Project Settings.

#include "Headquarters/Plugins/SupplyAreaComponent.h"

#pragma region Multiplayer Libraries

#include "Net/UnrealNetwork.h"

#pragma endregion

#pragma region InProjectIncludes

#include "Headquarters/Plugins/DataAssets/SupplyAreaDA.h"
#include "Motorcycle/Interfaces/SupplyInterface.h"

#pragma endregion

USupplyAreaComponent::USupplyAreaComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(true);

	AreaMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AreaMesh"));
	AreaMesh->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);

	AreaMesh->SetGenerateOverlapEvents(true);
	AreaMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	AreaMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	AreaMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
}

void USupplyAreaComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USupplyAreaComponent, AreaMesh);
}

void USupplyAreaComponent::BeginPlay()
{
	Super::BeginPlay();
}

void USupplyAreaComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void USupplyAreaComponent::InitializeArea(UPrimaryDataAsset* PluginData)
{
	Super::InitializeArea(PluginData);
	auto SupplyAreaData = Cast<USupplyAreaDA>(PluginData);
	if (SupplyAreaData)
	{
		AreaMesh->SetStaticMesh(SupplyAreaData->NewAreaMesh);
		SetRelativeTransform(SupplyAreaData->AreaTransform);
	}

	AreaMesh->OnComponentBeginOverlap.AddDynamic(
		this,
		&USupplyAreaComponent::OnMeshBeginOverlap
	);
}

void USupplyAreaComponent::OnMeshBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (ISupplyInterface* SupplyInterface = Cast<ISupplyInterface>(OtherActor))
	{
		SupplyInterface->SupplySoldier();
	}
}


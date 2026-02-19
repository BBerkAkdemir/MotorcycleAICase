#include "Headquarters/Plugins/SupplyAreaComponent.h"

#pragma region InProjectIncludes

#include "Headquarters/Plugins/DataAssets/SupplyAreaDA.h"
#include "Motorcycle/Interfaces/SupplyInterface.h"

#pragma endregion

USupplyAreaComponent::USupplyAreaComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(true);
}

void USupplyAreaComponent::BeginPlay()
{
	Super::BeginPlay();
}

void USupplyAreaComponent::InitializeArea(UPrimaryDataAsset* PluginData)
{
	Super::InitializeArea(PluginData);
	auto SupplyAreaData = Cast<USupplyAreaDA>(PluginData);
	if (!SupplyAreaData) return;

	AreaMesh = NewObject<UStaticMeshComponent>(GetOwner(), NAME_None, RF_Transient);
	AreaMesh->SetupAttachment(this);
	AreaMesh->SetStaticMesh(SupplyAreaData->NewAreaMesh);
	AreaMesh->SetGenerateOverlapEvents(true);
	AreaMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	AreaMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	AreaMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	AreaMesh->RegisterComponent();

	SetRelativeTransform(SupplyAreaData->AreaTransform);

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

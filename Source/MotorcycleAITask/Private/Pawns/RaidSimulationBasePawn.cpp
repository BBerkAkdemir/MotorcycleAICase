// Fill out your copyright notice in the Description page of Project Settings.

#include "Pawns/RaidSimulationBasePawn.h"
#include "Headquarters/Headquarters.h"
#include "NiagaraFunctionLibrary.h"

#pragma region Multiplayer Libraries

#include "Net/UnrealNetwork.h"

#pragma endregion

#pragma region AIPerception

#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISense_Sight.h"

#pragma endregion

ARaidSimulationBasePawn::ARaidSimulationBasePawn()
{
	PrimaryActorTick.bCanEverTick = true;
	Health = MaxHealth;
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	bReplicates = true;

	PerceptionStimuliSource = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("PerceptionStimuliSource"));
	PerceptionStimuliSource->bAutoRegister = true;
	PerceptionStimuliSource->RegisterForSense(TSubclassOf<UAISense>(UAISense_Sight::StaticClass()));
}

void ARaidSimulationBasePawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ARaidSimulationBasePawn, AI_ID);
	DOREPLIFETIME(ARaidSimulationBasePawn, Health);
	DOREPLIFETIME(ARaidSimulationBasePawn, PoolState);
}

void ARaidSimulationBasePawn::BeginPlay()
{
	Super::BeginPlay();
}

void ARaidSimulationBasePawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ARaidSimulationBasePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

bool ARaidSimulationBasePawn::DamageControl(float BaseDamage, FName HitBoneName, FVector const& HitFromDirection, FVector const& ImpactNormal)
{
	if (!HasAuthority() || PoolState == EPawnPoolState::Dead || PoolState == EPawnPoolState::InPool)
	{
		return false;
	}

	Health = FMath::Clamp(Health - BaseDamage, 0.0f, MaxHealth);

	if (Health <= 0.0f)
	{
		PoolState = EPawnPoolState::Dead;
		Internal_OnDead(HitBoneName, ImpactNormal);
		MulticastRPC_OnDead(HitBoneName, ImpactNormal);

		GetWorldTimerManager().SetTimer(TH_DeathToPool, this, &ARaidSimulationBasePawn::ReturnToPoolAfterDeath, DeathToPoolDelay, false);
		return true;
	}

	return false;
}

void ARaidSimulationBasePawn::OnRep_PoolState()
{
	switch (PoolState)
	{
	case EPawnPoolState::InPool:
		Internal_PushInThePool();
		break;
	case EPawnPoolState::Alive:
		Internal_PullFromThePool(GetActorLocation());
		break;
	case EPawnPoolState::Dead:
		Internal_OnDead(NAME_None, FVector::ZeroVector);
		break;
	default:
		break;
	}
}

void ARaidSimulationBasePawn::MulticastRPC_OnDead_Implementation(FName HitBoneName, FVector ImpactNormal)
{
	if (HasAuthority())
	{
		return;
	}
	Internal_OnDead(HitBoneName, ImpactNormal);
}

void ARaidSimulationBasePawn::ReturnToPoolAfterDeath()
{
	if (HasAuthority())
	{
		PushInThePool();
	}
}

void ARaidSimulationBasePawn::CancelPendingPoolReturn()
{
	GetWorldTimerManager().ClearTimer(TH_DeathToPool);
}

void ARaidSimulationBasePawn::MulticastRPC_OnFireVisual_Implementation(FVector MuzzleLocation, FVector HitLocation)
{
	if (MuzzleFlashEffect)
	{
		USceneComponent* MuzzleComp = GetMuzzleComponent();
		if (MuzzleComp)
		{
			UNiagaraFunctionLibrary::SpawnSystemAttached(MuzzleFlashEffect, MuzzleComp, NAME_None, FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTarget, true);
		}
		else
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, MuzzleFlashEffect, MuzzleLocation);
		}
	}
}

void ARaidSimulationBasePawn::MulticastRPC_OnHitVisual_Implementation(FVector HitLocation, FVector HitNormal)
{
	if (HitBloodEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, HitBloodEffect, HitLocation, HitNormal.Rotation());
	}
}

void ARaidSimulationBasePawn::Internal_OnDead(FName HitBoneName, FVector ImpactNormal)
{
}

void ARaidSimulationBasePawn::PushInThePool()
{
	if (HasAuthority())
	{
		PoolState = EPawnPoolState::InPool;
	}
	Internal_PushInThePool();
	MulticastRPC_PushInThePool();
}

void ARaidSimulationBasePawn::PullFromThePool(FVector PullLocation)
{
	if (HasAuthority())
	{
		PoolState = EPawnPoolState::Alive;
		Health = MaxHealth;
	}
	Internal_PullFromThePool(PullLocation);
	MulticastRPC_PullFromThePool(PullLocation);
}

void ARaidSimulationBasePawn::MulticastRPC_PushInThePool_Implementation()
{
	if (HasAuthority())
	{
		return;
	}
	Internal_PushInThePool();
}

void ARaidSimulationBasePawn::MulticastRPC_PullFromThePool_Implementation(FVector PullLocation)
{
	if (HasAuthority())
	{
		return;
	}
	Internal_PullFromThePool(PullLocation);
}

void ARaidSimulationBasePawn::Internal_PushInThePool()
{
	if (PerceptionStimuliSource && PerceptionStimuliSource->IsRegistered())
	{
		PerceptionStimuliSource->UnregisterComponent();
	}
}

void ARaidSimulationBasePawn::Internal_PullFromThePool(FVector PullLocation)
{
	if (PerceptionStimuliSource && !PerceptionStimuliSource->IsRegistered() && PerceptionStimuliSource->bAutoRegister)
	{
		PerceptionStimuliSource->RegisterComponent();
	}
}

void ARaidSimulationBasePawn::SetOwningHeadquarters(AHeadquarters* HQ)
{
	CachedHeadquarters = HQ;
}

void ARaidSimulationBasePawn::ActorAttachToComponent(USceneComponent* AttachedComponent)
{
	AttachToComponent(AttachedComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
}

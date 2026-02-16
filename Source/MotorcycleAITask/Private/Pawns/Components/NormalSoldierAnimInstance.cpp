// Fill out your copyright notice in the Description page of Project Settings.


#include "Pawns/Components/NormalSoldierAnimInstance.h"
#include "Pawns/NormalSoldierPawn.h"

void UNormalSoldierAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	ANormalSoldierPawn* SoldierPawn = Cast<ANormalSoldierPawn>(TryGetPawnOwner());
	if (SoldierPawn)
	{
		bIsFireStart = SoldierPawn->GetIsFiring();
		Target = SoldierPawn->GetFireTarget();
	}

	if (Target && GetOwningActor())
	{
		float DeltaYaw = FMath::FindDeltaAngleDegrees(
			GetOwningActor()->GetActorRotation().Yaw,
			(Target->GetActorLocation() - GetOwningActor()->GetActorLocation()).Rotation().Yaw
		);
		Angle = FMath::Clamp(DeltaYaw, -90.f, 90.f);
	}
	else
	{
		Angle = 0.f;
	}
}


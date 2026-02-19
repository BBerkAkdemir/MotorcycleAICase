// Fill out your copyright notice in the Description page of Project Settings.


#include "Motorcycle/Components/MotorcycleMovementComponent.h"

#pragma region KismetLibraries

#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

#pragma endregion

#pragma region UnrealComponents

#include "Components/CapsuleComponent.h"
#include "GameFramework/FloatingPawnMovement.h"

#pragma endregion

#pragma region InProjectIncludes

#include "Motorcycle/MotorcyclePawn.h"
#include "Motorcycle/SplinePathActor.h"
#include "Structs/RaidSimulationProjectStructs.h"

#pragma endregion


UMotorcycleMovementComponent::UMotorcycleMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(true);
}

void UMotorcycleMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerPawn = StaticCast<AMotorcyclePawn*>(GetOwner());

	CollisionQueryParams.AddIgnoredActor(GetOwner());

	if (bIsAutoActiveMovement)
	{
		ActivateMovement();
	}
}

void UMotorcycleMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (GetOwner()->HasAuthority())
	{
		if (FollowMode == EMotorcycleFollowMode::FollowSpline && CachedSplinePath)
		{
			UpdateSplineTarget();
		}

		CalculateSpeed(DeltaTime);
		CalculateMeshRotation(DeltaTime);
		CalculateOwnerRotation(DeltaTime);
		MoveMotorcycle();
	}
	else
	{
		float Delta = FMath::Clamp(float(DeltaTime) / 250.0f, 0.0f, 1.0f);
		OwnerPawn->SetActorLocation(FMath::Lerp(OwnerPawn->GetActorLocation(), NewRepLocation, Delta));
		OwnerPawn->SetActorRotation(UKismetMathLibrary::RLerp(OwnerPawn->GetActorRotation(), NewRepRotation, Delta, true));
		OwnerPawn->GetMesh()->SetRelativeRotation(UKismetMathLibrary::RLerp(OwnerPawn->GetMesh()->GetRelativeRotation(), NewRepMeshRotation, Delta, true));
		OwnerPawn->GetPawnMovement()->Velocity = FMath::Lerp(OwnerPawn->GetPawnMovement()->Velocity, NewRepVelocity, Delta);
	}
}

void UMotorcycleMovementComponent::ApplyMovementData(const FMotorcycleAIReplicationData& Data)
{
	FVector TempRepLocation = NewRepLocation;
	NewRepLocation = FVector(float(Data.PosX), float(Data.PosY), float(Data.PosZ));
	NewRepRotation = FRotator(0, float(Data.ActorRotation_WithYaw) * 1.41f, 0);
	NewRepMeshRotation = FRotator(float(Data.MeshRotation_WithPitch) * 1.41f, float(Data.MeshRotation_WithYaw) * 1.41f, float(Data.MeshRotation_WithRoll) * 1.41f);
	NewRepVelocity = NewRepLocation - TempRepLocation;
}

void UMotorcycleMovementComponent::ActivateMovement()
{
	if (GetOwner()->HasAuthority())
	{
		if (!IsComponentTickEnabled())
		{
			DoAsyncTrace();
		}
	}
	SetComponentTickEnabled(true);
}

void UMotorcycleMovementComponent::DeactivateMovement()
{
	SetComponentTickEnabled(false);
}

FVector UMotorcycleMovementComponent::GetFollowTargetLocation() const
{
	if (FollowMode == EMotorcycleFollowMode::FollowSpline && CachedSplinePath)
	{
		return SplineTargetLocation;
	}

	if (IsValid(FollowObject))
	{
		return FollowObject->GetActorLocation();
	}

	return OwnerPawn->GetActorLocation() + OwnerPawn->GetActorForwardVector() * 100.f;
}

void UMotorcycleMovementComponent::UpdateSplineTarget()
{
	float DistToTarget = FVector::Dist2D(OwnerPawn->GetActorLocation(), SplineTargetLocation);

	if (DistToTarget < SplineReachThreshold)
	{
		if (bSplineForward)
		{
			CurrentSplineDistance += SplineLookaheadDistance;
		}
		else
		{
			CurrentSplineDistance -= SplineLookaheadDistance;
		}

		float SplineLength = CachedSplinePath->GetSplineLength();

		if (bSplineForward && CurrentSplineDistance >= SplineLength)
		{
			if (bLoopSpline)
			{
				CurrentSplineDistance = SplineLookaheadDistance;
			}
			else
			{
				CurrentSplineDistance = SplineLength;
				FollowMode = EMotorcycleFollowMode::FollowActor;
				OnSplineEndReached.Broadcast();
				return;
			}
		}
		else if (!bSplineForward && CurrentSplineDistance <= 0.f)
		{
			CurrentSplineDistance = 0.f;
			FollowMode = EMotorcycleFollowMode::FollowActor;
			OnSplineEndReached.Broadcast();
			return;
		}
	}

	SplineTargetLocation = CachedSplinePath->GetLocationAtDistance(CurrentSplineDistance);
}

void UMotorcycleMovementComponent::CalculateSpeed(float DeltaTime)
{
	FVector TargetLocation = GetFollowTargetLocation();
	FVector ForwardVector = OwnerPawn->GetActorForwardVector();
	FVector Directon = UKismetMathLibrary::GetDirectionUnitVector(OwnerPawn->GetActorLocation(), TargetLocation);
	float DotAngle = UKismetMathLibrary::DotProduct2D(FVector2D(ForwardVector.X, ForwardVector.Y), FVector2D(Directon.X, Directon.Y));
	bool bIsBackFront = DotAngle >= MinBackwardSpeed;

	float CalculatedNewSpeed = UKismetMathLibrary::SelectFloat(
		UKismetMathLibrary::FClamp(DotAngle, MinForwardSpeed, 1.0f),
		UKismetMathLibrary::FClamp(DotAngle, -1.0f, MinBackwardSpeed),
		bIsBackFront);

	CurrentSpeed = UKismetMathLibrary::Lerp(CurrentSpeed, CalculatedNewSpeed, UKismetMathLibrary::SelectFloat(DeltaTime, DeltaTime * 3, bIsBackFront));
}

void UMotorcycleMovementComponent::CalculateMeshRotation(float DeltaTime)
{
	FVector ProjectedForward = UKismetMathLibrary::ProjectVectorOnToPlane(OwnerPawn->GetActorForwardVector(), ImpactNormal);
	FVector ProjectedRight = UKismetMathLibrary::Cross_VectorVector(ImpactNormal, ProjectedForward);
	FRotator CalculatedRotation = UKismetMathLibrary::MakeRotationFromAxes(ProjectedForward, ProjectedRight, ImpactNormal);
	OwnerPawn->GetMesh()->SetRelativeRotation(
		UKismetMathLibrary::RLerp(OwnerPawn->GetMesh()->GetRelativeRotation(),
			CalculatedRotation,
			DeltaTime * MeshRotateSpeed,
			true));
}

void UMotorcycleMovementComponent::DoAsyncTrace()
{
	FTraceDelegate TraceDelegate;
	TraceDelegate.BindUObject(this, &UMotorcycleMovementComponent::OnTraceCompleted);

	GetWorld()->AsyncLineTraceByChannel(EAsyncTraceType::Single,
		OwnerPawn->GetActorLocation() + FVector(0, 0, 1000),
		OwnerPawn->GetActorLocation() + FVector(0, 0, -1000),
		ECollisionChannel::ECC_Visibility,
		CollisionQueryParams,
		FCollisionResponseParams::DefaultResponseParam,
		&TraceDelegate);
}

void UMotorcycleMovementComponent::OnTraceCompleted(const FTraceHandle& Handle, FTraceDatum& Data)
{
	if (!Data.OutHits.IsEmpty())
	{
		ImpactLocation = Data.OutHits[0].ImpactPoint;
		ImpactNormal = Data.OutHits[0].ImpactNormal;
	}
	if (IsComponentTickEnabled())
	{
		DoAsyncTrace();
	}
}

void UMotorcycleMovementComponent::CalculateOwnerRotation(float DeltaTime)
{
	FVector TargetLocation = GetFollowTargetLocation();
	FVector RightVector = OwnerPawn->GetActorRightVector();
	FVector Directon = UKismetMathLibrary::GetDirectionUnitVector(OwnerPawn->GetActorLocation(), TargetLocation);
	float DotAngle = UKismetMathLibrary::DotProduct2D(FVector2D(RightVector.X, RightVector.Y), FVector2D(Directon.X, Directon.Y));
	OwnerPawn->AddActorWorldRotation(FRotator(0, DotAngle * DeltaTime * OwnerRotateSpeed, 0));
}

void UMotorcycleMovementComponent::MoveMotorcycle()
{
	FVector ForwardVector = OwnerPawn->GetActorForwardVector();
	if (OwnerPawn->GetActorLocation().Z - ImpactLocation.Z - OwnerPawn->GetCapsule()->GetScaledCapsuleHalfHeight() > 10)
	{
		ForwardVector -= CurrentSpeed * FVector(0.0f, 0.0f, 50.0f);
	}
	OwnerPawn->AddMovementInput(ForwardVector, CurrentSpeed);
}

void UMotorcycleMovementComponent::SetSplinePath(ASplinePathActor* SplinePath, bool bForward, bool bLoop)
{
	if (!SplinePath)
	{
		SetFollowTarget(FollowObject);
		return;
	}

	CachedSplinePath = SplinePath;
	bSplineForward = bForward;
	bLoopSpline = bLoop;
	FollowMode = EMotorcycleFollowMode::FollowSpline;

	CurrentSplineDistance = CachedSplinePath->FindDistanceClosestToWorldLocation(OwnerPawn->GetActorLocation());

	if (bSplineForward)
	{
		CurrentSplineDistance = FMath::Min(CurrentSplineDistance + SplineLookaheadDistance, CachedSplinePath->GetSplineLength());
	}
	else
	{
		CurrentSplineDistance = FMath::Max(CurrentSplineDistance - SplineLookaheadDistance, 0.f);
	}

	SplineTargetLocation = CachedSplinePath->GetLocationAtDistance(CurrentSplineDistance);
}

void UMotorcycleMovementComponent::SetFollowTarget(AActor* Target)
{
	if (Target)
	{
		FollowObject = Target;
	}
	FollowMode = EMotorcycleFollowMode::FollowActor;
}

void UMotorcycleMovementComponent::AddIgnoredActorForTrace(AActor* Actor)
{
	if (Actor)
	{
		CollisionQueryParams.AddIgnoredActor(Actor);
	}
}

void UMotorcycleMovementComponent::ClearIgnoredCrewForTrace()
{
	CollisionQueryParams = FCollisionQueryParams();
	CollisionQueryParams.AddIgnoredActor(GetOwner());
}

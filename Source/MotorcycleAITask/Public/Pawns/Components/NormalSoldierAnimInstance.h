// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "NormalSoldierAnimInstance.generated.h"

UCLASS()
class MOTORCYCLEAITASK_API UNormalSoldierAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	AActor* Target;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float Angle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsFireStart;

public:

	void SetTarget(AActor* NewTarget) { Target = NewTarget; };
	void SetIsFireStart(bool IsFireStart) { bIsFireStart = IsFireStart; };

protected:

	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

};

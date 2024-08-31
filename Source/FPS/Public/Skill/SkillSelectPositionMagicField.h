// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FPSGameplayAbility.h"
#include "MagicFieldBase.h"
#include "SkillSelectPositionMagicField.generated.h"

#define TRACE_IGNORE_PAWN ECC_GameTraceChannel1

/**
 * 
 */
UCLASS()
class FPS_API USkillSelectPositionMagicField : public UFPSGameplayAbility
{
	GENERATED_BODY()

public:
	USkillSelectPositionMagicField();

	UPROPERTY(EditAnywhere)
	TSubclassOf<AMagicFieldBase> MagicFieldClass;

	UPROPERTY(EditAnywhere)
	float CastDistance;
	
	UPROPERTY(EditAnywhere)
	float SpawnHeightOffset;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	bool GetTargetLocation(FVector StartLocation, FVector ForwardVector, FVector& TargetLocation);
	
};

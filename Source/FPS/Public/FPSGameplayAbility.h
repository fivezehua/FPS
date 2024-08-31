// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FPSCharacterBase.h"
#include "Abilities/GameplayAbility.h"
#include "FPSGameplayAbility.generated.h"

/**
 * 
 */
UCLASS()
class FPS_API UFPSGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UFPSGameplayAbility();
	
public:
	UPROPERTY(EditAnywhere)
	EAbilityInputID AbilityInputID;
};

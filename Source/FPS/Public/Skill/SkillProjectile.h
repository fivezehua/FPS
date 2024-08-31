// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FPSGameplayAbility.h"
#include "ProjectileBase.h"
#include "Abilities/GameplayAbility.h"
#include "SkillProjectile.generated.h"

/**
 * 
 */
UCLASS()
class FPS_API USkillProjectile : public UFPSGameplayAbility
{
	GENERATED_BODY()

public:
	USkillProjectile();
	
	UPROPERTY(EditAnywhere)
	TSubclassOf<AProjectileBase> ProjectileClass;
	
protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};

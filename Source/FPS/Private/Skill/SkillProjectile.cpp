// Fill out your copyright notice in the Description page of Project Settings.


#include "Skill/SkillProjectile.h"

USkillProjectile::USkillProjectile()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
}

void USkillProjectile::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	if(CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		if(ProjectileClass)
		{
			AActor* Avatar = GetAvatarActorFromActorInfo();
			AFPSCharacterBase* FpsCharacter = Cast<AFPSCharacterBase>(Avatar);
			FActorSpawnParameters SpawnInfo;
			SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			SpawnInfo.Instigator = FpsCharacter;
			FTransform Transform = FpsCharacter->GetCameraTransform();
			GetWorld()->SpawnActor<AProjectileBase>(ProjectileClass, Transform, SpawnInfo);
		}
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}

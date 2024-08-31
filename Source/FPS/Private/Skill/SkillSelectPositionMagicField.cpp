// Fill out your copyright notice in the Description page of Project Settings.


#include "Skill/SkillSelectPositionMagicField.h"

#include "Kismet/KismetSystemLibrary.h"

USkillSelectPositionMagicField::USkillSelectPositionMagicField()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
}

void USkillSelectPositionMagicField::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                                     const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                                     const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	if(CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		if(MagicFieldClass)
		{
			AActor* Avatar = GetAvatarActorFromActorInfo();
			AFPSCharacterBase* FpsCharacter = Cast<AFPSCharacterBase>(Avatar);
			FActorSpawnParameters SpawnInfo;
			SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			SpawnInfo.Instigator = FpsCharacter;
			FVector TargetLocation;
			bool GetSuccess = GetTargetLocation(FpsCharacter->GetCameraLocation(), FpsCharacter->GetCameraForwardVector(), TargetLocation);
			if(GetSuccess)
			{
				TargetLocation.Z += SpawnHeightOffset;
				GetWorld()->SpawnActor<AMagicFieldBase>(MagicFieldClass, TargetLocation, FRotator::ZeroRotator, SpawnInfo);
			}
		}
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}

bool USkillSelectPositionMagicField::GetTargetLocation(FVector StartLocation, FVector ForwardVector, FVector& TargetLocation)
{
	FVector EndLocation = StartLocation + ForwardVector * CastDistance;

	TArray<AActor*> IgnoreArray;
	IgnoreArray.Add(GetAvatarActorFromActorInfo());
	FHitResult HitResult;
	bool HitSuccess = UKismetSystemLibrary::LineTraceSingle(GetWorld(), StartLocation, EndLocation,  UEngineTypes::ConvertToTraceType(TRACE_IGNORE_PAWN), false,
	IgnoreArray, EDrawDebugTrace::None, HitResult, true, FLinearColor::Red,
	FLinearColor::Green, 3.f);
	if(HitSuccess)
	{
		TargetLocation = HitResult.Location;
		UKismetSystemLibrary::PrintString(GetWorld(), FString::Printf(TEXT("%f %f %f"), TargetLocation.X, TargetLocation.Y, TargetLocation.Z));
		return true;
	}
	else
	{
		return false;
	}
}

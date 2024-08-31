// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponBaseClient.generated.h"

UCLASS()
class FPS_API AWeaponBaseClient : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWeaponBaseClient();

	UPROPERTY(EditAnywhere, BlueprintReadWrite);
	USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(EditAnywhere);
	UAnimMontage* ClientArmsFireAnimMontage;

	UPROPERTY(EditAnywhere);
	UAnimMontage* ClientArmsReloadAnimMontage;

	UPROPERTY(EditAnywhere);
	USoundBase* FireSound;

	UPROPERTY(EditAnywhere);
	UParticleSystem* MuzzleFlash;

	UPROPERTY(EditAnywhere);
	TSubclassOf<UCameraShakeBase> CameraShakeClass;
	
	UPROPERTY(EditAnywhere, Category = "Recoil")
	UCurveFloat* VerticalRecoilCurve;

	UPROPERTY(EditAnywhere, Category = "Recoil")
	UCurveFloat* HorizontalRecoilCurve;

	UPROPERTY(EditAnywhere)
	FName ClientArmsWeaponSocketName;
	
	UPROPERTY(EditAnywhere, Category = "FPGunAnimation")
	int FPArmsBlendPose;

	void DisplayWeaponEffect();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "FPGunAnimation")
	void PlayFireAnimation();

	UFUNCTION(BlueprintImplementableEvent, Category = "FPGunAnimation")
	void PlayReloadAnimation();
};

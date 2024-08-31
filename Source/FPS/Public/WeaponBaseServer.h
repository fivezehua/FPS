// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WeaponBaseClient.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Actor.h"
#include "WeaponBaseServer.generated.h"

UENUM()
enum class EWeaponKind : uint8
{
	Primary UMETA(DisplayName = "Primary"),
	Secondary UMETA(DisplayName = "Secondary"),
};

UENUM()
enum class EWeaponType : uint8
{
	AK47 UMETA(DisplayName = "AK47"),
	M4A1 UMETA(DisplayName = "M4A1"),
	MP7 UMETA(DisplayName = "MP7"),
	DesertEagle UMETA(DisplayName = "DesertEagle"),
};


UCLASS()
class FPS_API AWeaponBaseServer : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWeaponBaseServer();

	UPROPERTY(EditAnywhere)
	EWeaponKind KindOfWeapon;
	
	UPROPERTY(EditAnywhere)
	EWeaponType TypeOfWeapon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(EditAnywhere)
	USphereComponent* SphereCollision;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AWeaponBaseClient> ClientWeaponBaseBPClass;
	
	UPROPERTY(EditAnywhere);
	UParticleSystem* MuzzleFlash;

	UPROPERTY(EditAnywhere);
	USoundBase* FireSound;

	UPROPERTY(EditAnywhere, Replicated, Category = "Ammo");
	int32 GunCurrentAmmo;
	
	UPROPERTY(EditAnywhere, Replicated, Category = "Ammo");
	int32 ClipCurrentAmmo;
	
	UPROPERTY(EditAnywhere, Category = "Ammo");
	int32 MaxClipAmmo;
	
	UPROPERTY(EditAnywhere, Category = "AnimMontage")
	UAnimMontage* ServerTpBodiesFireAnimMontage;

	UPROPERTY(EditAnywhere, Category = "AnimMontage")
	UAnimMontage* ServerTpBodiesReloadAnimMontage;

	// 因为第三人称换弹和第一人称换弹动作并不匹配，所以时间会有出入，只能在服务端存个第一人称的蒙太奇以获取时间
	UPROPERTY(EditAnywhere);
	UAnimMontage* ClientArmsReloadAnimMontage;

	UPROPERTY(EditAnywhere)
	float BulletDistance;

	UPROPERTY(EditAnywhere)
	UMaterialInterface* BulletHoleMaterial;

	UPROPERTY(EditAnywhere)
	float BaseDamage;

	UPROPERTY(EditAnywhere)
	bool IsAutomatic;

	UPROPERTY(EditAnywhere)
	float AutomaticFireRate;
	
	UPROPERTY(EditAnywhere)
	float MovingFireRandomRange;

	UPROPERTY(EditAnywhere, Category = "Spread")
	float ResetPistolSpreadCallBackRate;

	UPROPERTY(EditAnywhere, Category = "Spread")
	float SpreadMinStep;
	
	UPROPERTY(EditAnywhere, Category = "Spread")
	float SpreadMaxStep;
	
	UFUNCTION(NetMulticast, Reliable, WithValidation)
	void MultiFireEffect();
	void MultiFireEffect_Implementation();
	bool MultiFireEffect_Validate();

	UFUNCTION()
	void OnOtherBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);

	UFUNCTION()
	void EquipWeapon();

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const;

	UFUNCTION(BlueprintImplementableEvent, Category = "FPGunAnimation")
	void PlayFireAnimation();
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};

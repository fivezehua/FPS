// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "MultiFPSPlayerController.h"
#include "WeaponBaseServer.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Character.h"
#include "FPSCharacterBase.generated.h"

UENUM()
enum class EAbilityInputID: uint8
{
	Skill_1 UMETA(DisplayName = "Skill_1"),
	Skill_2 UMETA(DisplayName = "Skill_2"),
};

UCLASS()
class FPS_API AFPSCharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AFPSCharacterBase();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION()
	void DelayBeginPlayCallback();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	class UAbilitySystemComponent* AbilitySystemComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilities")
	TArray<TSubclassOf<class UGameplayAbility>> PreloadedAbilities;

private:
	UPROPERTY(Category=Character, VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	UCameraComponent* PlayerCamera;
	
	UPROPERTY(Category=Character, VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	USkeletalMeshComponent* FPArmsMesh;

	UPROPERTY(Category=Character, VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	USkeletalMeshComponent* TPBodiesMesh;

	UPROPERTY(Category=Character, VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	UCharacterMovementComponent* CharacterMovementComponent;

	UPROPERTY(Category=Character, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	UAnimInstance* ClientArmsAnimBP;

	UPROPERTY(Category=Character, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	UAnimInstance* ServerBodiesAnimBP;

	UPROPERTY(Category=Character, EditAnywhere, meta=(AllowPrivateAccess = "true"))
	UAnimMontage* ServerTpBodiesSwitchPrimaryAnimMontage;

	UPROPERTY(Category=Character, EditAnywhere, meta=(AllowPrivateAccess = "true"))
	UAnimMontage* ServerTpBodiesSwitchSecondaryAnimMontage;
	
	// 通过记录服务器当前所持武器类型来保证客户端第一人称初始化时装备的一定是主武器
	UPROPERTY(meta=(AllowPrivateAccess = "true"), Replicated)
	EWeaponKind ServerHoldWeaponKind;
	
	UPROPERTY(meta=(AllowPrivateAccess = "true"))
	AWeaponBaseServer* ServerHoldWeapon;

	UPROPERTY(meta=(AllowPrivateAccess = "true"))
	AWeaponBaseClient* ClientHoldWeapon;

	UPROPERTY(meta=(AllowPrivateAccess = "true"))
	AWeaponBaseServer* ServerPrimaryWeapon;
	
	UPROPERTY(meta=(AllowPrivateAccess = "true"))
	AWeaponBaseClient* ClientPrimaryWeapon;

	UPROPERTY(meta=(AllowPrivateAccess = "true"))
	AWeaponBaseServer* ServerSecondaryWeapon;
	
	UPROPERTY(meta=(AllowPrivateAccess = "true"))
	AWeaponBaseClient* ClientSecondaryWeapon;
	
	AWeaponBaseServer* GetCurrentServerTPBodiesWeaponActor();
	
	AWeaponBaseClient* GetCurrentClientFPArmsWeaponActor();

	UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	AMultiFPSPlayerController* FPSPlayerController;

	UPROPERTY(EditAnywhere, meta=(AllowPrivateAccess = "true"), Category = "Health")
	int MaxHealth;
	float Health;

	UPROPERTY(EditAnywhere, meta=(AllowPrivateAccess = "true"), Category = "Damage")
	float HeadDamageRatio;
	
	UPROPERTY(EditAnywhere, meta=(AllowPrivateAccess = "true"), Category = "Damage")
	float BodyDamageRatio;
	
	UPROPERTY(EditAnywhere, meta=(AllowPrivateAccess = "true"), Category = "Damage")
	float ArmDamageRatio;
	
	UPROPERTY(EditAnywhere, meta=(AllowPrivateAccess = "true"), Category = "Damage")
	float LegDamageRatio;

	UPROPERTY(EditAnywhere)
	EWeaponType TestStartWeapon;

	UPROPERTY(EditAnywhere)
	TArray<EWeaponType> InitPrimaryWeaponList;

	UPROPERTY(EditAnywhere)
	TArray<EWeaponType> InitSecondaryWeaponList;
	
	void StartWithKindOfWeapon();
	void InitPrimaryWeapon();
	void InitSecondaryWeapon();
	void PurchaseWeapon(EWeaponType WeaponType);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void MoveForward(float AxisValue);
	void MoveRight(float AxisValue);
	void LowSpeedWalkAction();
	void NormalSpeedWalkAction();

	void JumpAction();
	void StopJumpAction();

	void InputFirePressed();
	void InputFireReleased();

	void InputReload();

	void InputSwitchPrimary();
	void InputSwitchSecondary();
	
	void BindAbilitySystemComponentInput();
	
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintImplementableEvent)
	void UpdateFPArmsBlendPose(int BlendPose);
	
	void EquipPrimary(AWeaponBaseServer* WeaponBaseServer);
	void EquipSecondary(AWeaponBaseServer* WeaponBaseServer);
	
	// 网络同步相关
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerLowSpeedWalkAction();
	void ServerLowSpeedWalkAction_Implementation();
	bool ServerLowSpeedWalkAction_Validate();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerNormalSpeedWalkAction();
	void ServerNormalSpeedWalkAction_Implementation();
	bool ServerNormalSpeedWalkAction_Validate();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerFireRifleWeapon(FVector CameraLocation, FRotator CameraRotator, bool IsMoving);
	void ServerFireRifleWeapon_Implementation(FVector CameraLocation, FRotator CameraRotation, bool IsMoving);
	bool ServerFireRifleWeapon_Validate(FVector CameraLocation, FRotator CameraRotation, bool IsMoving);
	
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerFirePistolWeapon(FVector CameraLocation, FRotator CameraRotator, bool IsMoving);
	void ServerFirePistolWeapon_Implementation(FVector CameraLocation, FRotator CameraRotation, bool IsMoving);
	bool ServerFirePistolWeapon_Validate(FVector CameraLocation, FRotator CameraRotation, bool IsMoving);
	
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerStopFiring();
	void ServerStopFiring_Implementation();
	bool ServerStopFiring_Validate();
	
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerReloadPrimary();
	void ServerReloadPrimary_Implementation();
	bool ServerReloadPrimary_Validate();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerReloadSecondary();
	void ServerReloadSecondary_Implementation();
	bool ServerReloadSecondary_Validate();

	// 切枪相关
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSwitchPrimary();
	void ServerSwitchPrimary_Implementation();
	bool ServerSwitchPrimary_Validate();
	
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSwitchSecondary();
	void ServerSwitchSecondary_Implementation();
	bool ServerSwitchSecondary_Validate();

	UFUNCTION(NetMulticast, Reliable, WithValidation)
	void MultiFire();
	void MultiFire_Implementation();
	bool MultiFire_Validate();

	UFUNCTION(NetMulticast, Reliable, WithValidation)
	void MultiSpawnBulletHole(FVector Location, FRotator Rotation);
	void MultiSpawnBulletHole_Implementation(FVector Location, FRotator Rotation);
	bool MultiSpawnBulletHole_Validate(FVector Location, FRotator Rotation);

	UFUNCTION(NetMulticast, Reliable, WithValidation)
	void MultiReload();
	void MultiReload_Implementation();
	bool MultiReload_Validate();

	UFUNCTION(NetMulticast, Reliable, WithValidation)
	void MultiSwitchWeapon(EWeaponKind WeaponKind);
	void MultiSwitchWeapon_Implementation(EWeaponKind WeaponKind);
	bool MultiSwitchWeapon_Validate(EWeaponKind WeaponKind);

	UFUNCTION(Client, Reliable)
	void ClientEquipFPArmsPrimary();

	UFUNCTION(Client, Reliable)
	void ClientEquipFPArmsSecondary();

	UFUNCTION(Client, Reliable)
	void ClientSwitchFPArmsPrimary();

	UFUNCTION(Client, Reliable)
	void ClientSwitchFPArmsSecondary();

	UFUNCTION(Client, Reliable)
	void ClientFire();

	UFUNCTION(Client, Reliable)
	void ClientUpdateAmmoUI(int32 ClipCurrentAmmo, int32 GunCurrentAmmo);

	UFUNCTION(Client, Reliable)
	void ClientUpdateHealthUI(float NewHealth, int CurMaxHealth);

	UFUNCTION(Client, Reliable)
	void ClientRecoil();

	UFUNCTION(Client, Reliable)
	void ClientReload();

	UFUNCTION(Client, Reliable)
	void ClientOnDead();
	
	// 开火相关
	FTimerHandle AutomaticFireTimerHandle;
	float LastFireTimeStamp;
	void AutomaticFire();
	
	void FireWeaponPrimary();
	void FireWeaponPrimaryImplementation();
	void StopFirePrimary();
	void RifleLineTrace(FVector CameraLocation, FRotator CameraRotator, bool IsMoving);

	void FireWeaponSecondary();
	void StopFireSecondary();
	void PistolLineTrace(FVector CameraLocation, FRotator CameraRotator, bool IsMoving);
	FRotator CalPistolSpreadRotation(FRotator CameraRotation);
	float PistolSpreadMin = 0;
	float PistolSpreadMax = 0;
	
	UFUNCTION()
	void DelayResetPistolSpreadCallBack();

	// 后坐力
	float NewVerticalRecoilAmount;
	float OldVerticalRecoilAmount;
	float VerticalRecoilAmount;

	float NewHorizontalRecoilAmount;
	float OldHorizontalRecoilAmount;
	float HorizontalRecoilAmount;

	float RecoilXCoordPerFire;
	void ResetRecoil();

	UPROPERTY(Replicated)
	bool IsFiring;

	UPROPERTY(Replicated)
	bool IsReloading;

	UFUNCTION()
	void DelayPlayArmReloadCallBack();

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const;

	void DamagePlayer(AActor* DamagedActor, FVector& HitFromDirection, FHitResult& HitInfo);

	void DamagePlayerBySkill(AActor* DamagedActor, float BaseDamage);

	UFUNCTION()
	void OnHit(AActor* DamagedActor, float Damage, class AController* InstigatedBy, FVector HitLocation, class UPrimitiveComponent* FHitComponent, FName BoneName, FVector ShotFromDirection, const class UDamageType* DamageType, AActor* DamageCauser);

	UFUNCTION()
	void OnHitBySkill(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);

	void OnDead(AActor* DamageCauser);

	UFUNCTION()
	bool IsDead();

	void DestroyWeaponOnDead();

	// 切枪相关
	void SwitchPrimary();
	void SwitchSecondary();

	FVector GetCameraLocation();
	FRotator GetCameraRotation();
	FTransform GetCameraTransform();
	FVector GetCameraForwardVector();
};

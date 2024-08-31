// Fill out your copyright notice in the Description page of Project Settings.


#include "FPSCharacterBase.h"

#include "AbilitySystemComponent.h"
#include "FPSGameplayAbility.h"
#include "Components/DecalComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

// Sets default values
AFPSCharacterBase::AFPSCharacterBase()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	PlayerCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("PlayerCamera"));
	if(PlayerCamera)
	{
		PlayerCamera->SetupAttachment(RootComponent);
		PlayerCamera->bUsePawnControlRotation = true;
	}
	FPArmsMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FPArmsMesh"));
	if(FPArmsMesh)
	{
		FPArmsMesh->SetupAttachment(PlayerCamera);
		FPArmsMesh->SetOnlyOwnerSee(true);
	}
	TPBodiesMesh = this->GetMesh();
	TPBodiesMesh->SetOwnerNoSee(true);
	TPBodiesMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TPBodiesMesh->SetCollisionObjectType(ECC_Pawn);

	CharacterMovementComponent = Cast<UCharacterMovementComponent>(this->GetMovementComponent());
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
	
}

UAbilitySystemComponent* AFPSCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AFPSCharacterBase::DelayBeginPlayCallback()
{
	FPSPlayerController = Cast<AMultiFPSPlayerController>(GetController());
	if(FPSPlayerController)
	{
		FPSPlayerController->CreatePlayerUI();
	}
	else
	{
		FLatentActionInfo ActionInfo;
		ActionInfo.CallbackTarget = this;
		ActionInfo.ExecutionFunction = TEXT("DelayBeginPlayCallback");
		ActionInfo.UUID = FMath::Rand();
		ActionInfo.Linkage = 0;
		UKismetSystemLibrary::Delay(this, 0.5, ActionInfo);
	}
}

AWeaponBaseServer* AFPSCharacterBase::GetCurrentServerTPBodiesWeaponActor()
{
	return ServerHoldWeapon;
}

AWeaponBaseClient* AFPSCharacterBase::GetCurrentClientFPArmsWeaponActor()
{
	return ClientHoldWeapon;
}

void AFPSCharacterBase::StartWithKindOfWeapon()
{
	if(GetWorld()->IsServer())
	{
		InitPrimaryWeapon();
		InitSecondaryWeapon();
	}
}

void AFPSCharacterBase::InitPrimaryWeapon()
{
	int32 PrimaryWeaponTypeIndex = UKismetMathLibrary::RandomIntegerInRange(0, InitPrimaryWeaponList.Num() - 1);
	EWeaponType PrimaryWeaponType = InitPrimaryWeaponList[PrimaryWeaponTypeIndex];
	PurchaseWeapon(PrimaryWeaponType);
}

void AFPSCharacterBase::InitSecondaryWeapon()
{
	int32 SecondaryWeaponTypeIndex = UKismetMathLibrary::RandomIntegerInRange(0, InitSecondaryWeaponList.Num() - 1);
	EWeaponType SecondaryWeaponType = InitSecondaryWeaponList[SecondaryWeaponTypeIndex];
	PurchaseWeapon(SecondaryWeaponType);
}

void AFPSCharacterBase::PurchaseWeapon(EWeaponType WeaponType)
{
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Owner = this;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	UClass* BlueprintVar = nullptr;
	switch (WeaponType)
	{
	case EWeaponType::AK47:
		{
			BlueprintVar = StaticLoadClass(AWeaponBaseServer::StaticClass(), nullptr, TEXT("Blueprint'/Game/Blueprint/Weapon/AK47/ServerBP_AK47.ServerBP_AK47_C'"));
		}
		break;
	case EWeaponType::M4A1:
		{
			BlueprintVar = StaticLoadClass(AWeaponBaseServer::StaticClass(), nullptr, TEXT("Blueprint'/Game/Blueprint/Weapon/M4A1/ServerBP_M4A1.ServerBP_M4A1_C'"));
		}
		break;
	case EWeaponType::MP7:
		{
			BlueprintVar = StaticLoadClass(AWeaponBaseServer::StaticClass(), nullptr, TEXT("Blueprint'/Game/Blueprint/Weapon/MP7/ServerBP_MP7.ServerBP_MP7_C'"));
		}
		break;
	case EWeaponType::DesertEagle:
		{
			BlueprintVar = StaticLoadClass(AWeaponBaseServer::StaticClass(), nullptr, TEXT("Blueprint'/Game/Blueprint/Weapon/DesertEagle/ServerBP_DesertEagle.ServerBP_DesertEagle_C'"));
		}
		break;
	}
	if(BlueprintVar)
	{
		AWeaponBaseServer* ServerWeapon = GetWorld()->SpawnActor<AWeaponBaseServer>(BlueprintVar, GetActorTransform(), SpawnInfo);
		// 因为一spawn就会触发overlap，所以其实并不需要这两行代码
		// ServerWeapon->EquipWeapon();
		// EquipPrimary(ServerWeapon);
	}
}

// Called when the game starts or when spawned
void AFPSCharacterBase::BeginPlay()
{
	Super::BeginPlay();
	Health = MaxHealth;
	IsFiring = false;
	IsReloading = false;
	OnTakePointDamage.AddDynamic(this, &AFPSCharacterBase::OnHit);
	OnTakeAnyDamage.AddDynamic(this, &AFPSCharacterBase::OnHitBySkill);
	
	ClientArmsAnimBP = FPArmsMesh->GetAnimInstance();
	ServerBodiesAnimBP = TPBodiesMesh->GetAnimInstance();
	
	FPSPlayerController = Cast<AMultiFPSPlayerController>(GetController());
	if(FPSPlayerController)
	{
		FPSPlayerController->CreatePlayerUI();
	}
	else
	{
		FLatentActionInfo ActionInfo;
		ActionInfo.CallbackTarget = this;
		ActionInfo.ExecutionFunction = TEXT("DelayBeginPlayCallback");
		ActionInfo.UUID = FMath::Rand();
		ActionInfo.Linkage = 0;
		UKismetSystemLibrary::Delay(this, 0.5, ActionInfo);
	}
	
	StartWithKindOfWeapon();

	// GiveAbility要在权威控制的角色上才能调用，否则检查不通过会直接crash
	if(GetWorld()->IsServer())
	{
		if(AbilitySystemComponent)
		{
			if(PreloadedAbilities.Num() > 0)
			{
				for(auto i = 0; i < PreloadedAbilities.Num(); i++)
				{
					if(PreloadedAbilities[i] != nullptr)
					{
						UFPSGameplayAbility* FPSGameplayAbility = Cast<UFPSGameplayAbility>(PreloadedAbilities[i].GetDefaultObject());
						const int32 InputID = static_cast<int32>(FPSGameplayAbility->AbilityInputID);
						AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(FPSGameplayAbility, 1, InputID, this));
					}
				}
			}
			AbilitySystemComponent->InitAbilityActorInfo(this, this);
		}
	}
}

// Called every frame
void AFPSCharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AFPSCharacterBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	InputComponent->BindAxis(TEXT("MoveForward"), this, &AFPSCharacterBase::MoveForward);
	InputComponent->BindAxis(TEXT("MoveRight"), this, &AFPSCharacterBase::MoveRight);
	InputComponent->BindAction(TEXT("LowSpeedWalk"), IE_Pressed, this, &AFPSCharacterBase::LowSpeedWalkAction);
	InputComponent->BindAction(TEXT("LowSpeedWalk"), IE_Released, this, &AFPSCharacterBase::NormalSpeedWalkAction);

	InputComponent->BindAxis(TEXT("Turn"), this, &AFPSCharacterBase::AddControllerYawInput);
	InputComponent->BindAxis(TEXT("LookUp"), this, &AFPSCharacterBase::AddControllerPitchInput);

	InputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &AFPSCharacterBase::JumpAction);
	InputComponent->BindAction(TEXT("Jump"), IE_Released, this, &AFPSCharacterBase::StopJumpAction);

	InputComponent->BindAction(TEXT("Fire"), IE_Pressed, this, &AFPSCharacterBase::InputFirePressed);
	InputComponent->BindAction(TEXT("Fire"), IE_Released, this, &AFPSCharacterBase::InputFireReleased);

	InputComponent->BindAction(TEXT("Reload"), IE_Pressed, this, &AFPSCharacterBase::InputReload);

	InputComponent->BindAction(TEXT("SwitchPrimary"), IE_Pressed, this, &AFPSCharacterBase::InputSwitchPrimary);
	InputComponent->BindAction(TEXT("SwitchSecondary"), IE_Pressed, this, &AFPSCharacterBase::InputSwitchSecondary);
	
	BindAbilitySystemComponentInput();
}

void AFPSCharacterBase::ServerLowSpeedWalkAction_Implementation()
{
	CharacterMovementComponent->MaxWalkSpeed = 300;
}

bool AFPSCharacterBase::ServerLowSpeedWalkAction_Validate()
{
	return true;
}

void AFPSCharacterBase::ServerNormalSpeedWalkAction_Implementation()
{
	CharacterMovementComponent->MaxWalkSpeed = 600;
}

bool AFPSCharacterBase::ServerNormalSpeedWalkAction_Validate()
{
	return true;
}

void AFPSCharacterBase::ServerFireRifleWeapon_Implementation(FVector CameraLocation, FRotator CameraRotation,
	bool IsMoving)
{
	if(ServerPrimaryWeapon)
	{
		// 播放射击效果
		ServerPrimaryWeapon->MultiFireEffect();
		
		ServerPrimaryWeapon->ClipCurrentAmmo -= 1;

		// 播放身体动画蒙太奇
		MultiFire();
		
		ClientUpdateAmmoUI(ServerPrimaryWeapon->ClipCurrentAmmo, ServerPrimaryWeapon->GunCurrentAmmo);
	}

	IsFiring = true;
	RifleLineTrace(CameraLocation, CameraRotation, IsMoving);
}

bool AFPSCharacterBase::ServerFireRifleWeapon_Validate(FVector CameraLocation, FRotator CameraRotation, bool IsMoving)
{
	return true;
}

void AFPSCharacterBase::ServerFirePistolWeapon_Implementation(FVector CameraLocation, FRotator CameraRotation,
	bool IsMoving)
{
	if(ServerSecondaryWeapon->ClipCurrentAmmo > 0 && !IsReloading)
	{
		if(ServerSecondaryWeapon)
		{
			FLatentActionInfo ActionInfo;
			ActionInfo.CallbackTarget = this;
			ActionInfo.ExecutionFunction = TEXT("DelayResetPistolSpreadCallBack");
			ActionInfo.UUID = FMath::Rand();
			ActionInfo.Linkage = 0;
			UKismetSystemLibrary::Delay(this, ServerSecondaryWeapon->ResetPistolSpreadCallBackRate, ActionInfo);
		
			// 播放射击效果
			ServerSecondaryWeapon->MultiFireEffect();
		
			ServerSecondaryWeapon->ClipCurrentAmmo -= 1;

			// 播放身体动画蒙太奇
			MultiFire();
		
			ClientUpdateAmmoUI(ServerSecondaryWeapon->ClipCurrentAmmo, ServerSecondaryWeapon->GunCurrentAmmo);
		}

		IsFiring = true;
		PistolLineTrace(CameraLocation, CameraRotation, IsMoving);
	}
}

bool AFPSCharacterBase::ServerFirePistolWeapon_Validate(FVector CameraLocation, FRotator CameraRotation, bool IsMoving)
{
	return true;
}

void AFPSCharacterBase::ServerStopFiring_Implementation()
{
	IsFiring = false;
}

bool AFPSCharacterBase::ServerStopFiring_Validate()
{
	return true;
}

void AFPSCharacterBase::ServerReloadPrimary_Implementation()
{
	if(ServerPrimaryWeapon)
	{
		if(ServerPrimaryWeapon->GunCurrentAmmo > 0 && ServerPrimaryWeapon->ClipCurrentAmmo < ServerPrimaryWeapon->MaxClipAmmo)
		{
			IsReloading = true;
			ClientReload();
			MultiReload();
			FLatentActionInfo ActionInfo;
			ActionInfo.CallbackTarget = this;
			ActionInfo.ExecutionFunction = TEXT("DelayPlayArmReloadCallBack");
			ActionInfo.UUID = FMath::Rand();
			ActionInfo.Linkage = 0;
			UKismetSystemLibrary::Delay(this, ServerPrimaryWeapon->ClientArmsReloadAnimMontage->GetPlayLength(), ActionInfo);
		}
	}
}

bool AFPSCharacterBase::ServerReloadPrimary_Validate()
{
	return true;
}

void AFPSCharacterBase::ServerReloadSecondary_Implementation()
{
	if(ServerSecondaryWeapon)
	{
		if(ServerSecondaryWeapon->GunCurrentAmmo > 0 && ServerSecondaryWeapon->ClipCurrentAmmo < ServerSecondaryWeapon->MaxClipAmmo)
		{
			IsReloading = true;
			ClientReload();
			MultiReload();
			FLatentActionInfo ActionInfo;
			ActionInfo.CallbackTarget = this;
			ActionInfo.ExecutionFunction = TEXT("DelayPlayArmReloadCallBack");
			ActionInfo.UUID = FMath::Rand();
			ActionInfo.Linkage = 0;
			UKismetSystemLibrary::Delay(this, ServerSecondaryWeapon->ClientArmsReloadAnimMontage->GetPlayLength(), ActionInfo);
		}
	}
}

void AFPSCharacterBase::ServerSwitchPrimary_Implementation()
{
	if(ServerHoldWeaponKind != EWeaponKind::Primary && ServerPrimaryWeapon)
	{
		ServerHoldWeaponKind = EWeaponKind::Primary;
		ServerHoldWeapon = ServerPrimaryWeapon;
		ServerPrimaryWeapon->K2_AttachToComponent(TPBodiesMesh, TEXT("Weapon_Rifle"),
		EAttachmentRule::SnapToTarget,
		EAttachmentRule::SnapToTarget,
		EAttachmentRule::SnapToTarget,
		true);
		if(ServerSecondaryWeapon)
		{
			ServerSecondaryWeapon->K2_AttachToComponent(TPBodiesMesh, TEXT("Backup_Pistol"),
				EAttachmentRule::SnapToTarget,
				EAttachmentRule::SnapToTarget,
				EAttachmentRule::SnapToTarget,
				true);
		}
		ClientSwitchFPArmsPrimary();
		MultiSwitchWeapon(EWeaponKind::Primary);
	}
}

void AFPSCharacterBase::ServerSwitchSecondary_Implementation()
{
	if(ServerHoldWeaponKind != EWeaponKind::Secondary && ServerSecondaryWeapon)
	{
		ServerHoldWeaponKind = EWeaponKind::Secondary;
		ServerHoldWeapon = ServerSecondaryWeapon;
		ServerSecondaryWeapon->K2_AttachToComponent(TPBodiesMesh, TEXT("Weapon_Rifle"),
		EAttachmentRule::SnapToTarget,
		EAttachmentRule::SnapToTarget,
		EAttachmentRule::SnapToTarget,
		true);
		if(ServerPrimaryWeapon)
		{
			ServerPrimaryWeapon->K2_AttachToComponent(TPBodiesMesh, TEXT("Backup_Rifle"),
				EAttachmentRule::SnapToTarget,
				EAttachmentRule::SnapToTarget,
				EAttachmentRule::SnapToTarget,
				true);
		}
		ClientSwitchFPArmsSecondary();
		MultiSwitchWeapon(EWeaponKind::Secondary);
	}
}
bool AFPSCharacterBase::ServerSwitchSecondary_Validate()
{
	return true;
}

bool AFPSCharacterBase::ServerReloadSecondary_Validate()
{
	return true;
}

bool AFPSCharacterBase::ServerSwitchPrimary_Validate()
{
	return true;
}

void AFPSCharacterBase::MultiFire_Implementation()
{
	if(ServerBodiesAnimBP)
	{
		AWeaponBaseServer* CurrentServerWeapon = GetCurrentServerTPBodiesWeaponActor();
		if(CurrentServerWeapon)
		{
			ServerBodiesAnimBP->Montage_Play(CurrentServerWeapon->ServerTpBodiesFireAnimMontage);
			CurrentServerWeapon->PlayFireAnimation();
		}
	}
}

bool AFPSCharacterBase::MultiFire_Validate()
{
	return true;
}

void AFPSCharacterBase::MultiSpawnBulletHole_Implementation(FVector Location, FRotator Rotation)
{
	AWeaponBaseServer* CurrentServerWeapon = GetCurrentServerTPBodiesWeaponActor();
	if(CurrentServerWeapon)
	{
		// 贴花的rotation的forward方向需要和被击中点的法线方向保持一致，否则贴花显示异常
		UDecalComponent* BulletHole = UGameplayStatics::SpawnDecalAtLocation(GetWorld(), CurrentServerWeapon->BulletHoleMaterial, FVector(8, 8, 8),
			Location, Rotation, 10);
		if(BulletHole)
		{
			// 这样能在更远距离看见贴花
			BulletHole->SetFadeScreenSize(0.001);
		}
	}
}

bool AFPSCharacterBase::MultiSpawnBulletHole_Validate(FVector Location, FRotator Rotation)
{
	return true;
}

void AFPSCharacterBase::MultiReload_Implementation()
{
	AWeaponBaseServer* CurrentServerWeapon = GetCurrentServerTPBodiesWeaponActor();
	if(ServerBodiesAnimBP)
	{
		if(CurrentServerWeapon)
		{
			ServerBodiesAnimBP->Montage_Play(CurrentServerWeapon->ServerTpBodiesReloadAnimMontage);
		}
	}
}

bool AFPSCharacterBase::MultiReload_Validate()
{
	return true;
}

void AFPSCharacterBase::MultiSwitchWeapon_Implementation(EWeaponKind WeaponKind)
{
	if(ServerBodiesAnimBP)
	{
		if(WeaponKind == EWeaponKind::Primary)
		{
			ServerBodiesAnimBP->Montage_Play(ServerTpBodiesSwitchPrimaryAnimMontage);
		}
		else if(WeaponKind == EWeaponKind::Secondary)
		{
			ServerBodiesAnimBP->Montage_Play(ServerTpBodiesSwitchSecondaryAnimMontage);
		}
	}
}

bool AFPSCharacterBase::MultiSwitchWeapon_Validate(EWeaponKind WeaponKind)
{
	return true;
}

void AFPSCharacterBase::ClientSwitchFPArmsPrimary_Implementation()
{
	if(IsLocallyControlled())
	{
		if(ServerPrimaryWeapon)
		{
			if(ClientPrimaryWeapon)
			{
				ClientPrimaryWeapon->SetActorHiddenInGame(false);
				ClientUpdateAmmoUI(ServerPrimaryWeapon->ClipCurrentAmmo, ServerPrimaryWeapon->GunCurrentAmmo);
				UpdateFPArmsBlendPose(ClientPrimaryWeapon->FPArmsBlendPose);
			}
			if(ClientSecondaryWeapon)
			{
				ClientSecondaryWeapon->SetActorHiddenInGame(true);
			}
			ClientHoldWeapon = ClientPrimaryWeapon;
			ServerHoldWeapon = ServerPrimaryWeapon;
		}
	}
}

void AFPSCharacterBase::ClientSwitchFPArmsSecondary_Implementation()
{
	if(IsLocallyControlled())
	{
		if(ServerSecondaryWeapon)
		{
			if(ClientSecondaryWeapon)
			{
				ClientSecondaryWeapon->SetActorHiddenInGame(false);
				ClientUpdateAmmoUI(ServerSecondaryWeapon->ClipCurrentAmmo, ServerSecondaryWeapon->GunCurrentAmmo);
				UpdateFPArmsBlendPose(ClientSecondaryWeapon->FPArmsBlendPose);
			}
			if(ClientPrimaryWeapon)
			{
				ClientPrimaryWeapon->SetActorHiddenInGame(true);
			}
			ClientHoldWeapon = ClientSecondaryWeapon;
			ServerHoldWeapon = ServerSecondaryWeapon;
		}
	}
}

void AFPSCharacterBase::ClientFire_Implementation()
{
	// 枪械播放动画
	AWeaponBaseClient* CurrentClientWeapon = GetCurrentClientFPArmsWeaponActor();
	if(CurrentClientWeapon)
	{
		// 播放枪械动画
		CurrentClientWeapon->PlayFireAnimation();
		
		// 播放手臂动画蒙太奇
		UAnimMontage* ClientArmsFireMontage = CurrentClientWeapon->ClientArmsFireAnimMontage;
		ClientArmsAnimBP->Montage_Play(ClientArmsFireMontage);

		// 播放枪械效果
		CurrentClientWeapon->DisplayWeaponEffect();

		AMultiFPSPlayerController* MultiFPSPlayerController = Cast<AMultiFPSPlayerController>(GetController());
		if(MultiFPSPlayerController)
		{
			// 播放镜头抖动
			FPSPlayerController->PlayerCameraShake(CurrentClientWeapon->CameraShakeClass);

			// 播放准星扩散动画
			FPSPlayerController->DoCrosshairRecoil();
		}
	}
}

void AFPSCharacterBase::ClientUpdateAmmoUI_Implementation(int32 ClipCurrentAmmo, int32 GunCurrentAmmo)
{
	if(FPSPlayerController)
	{
		FPSPlayerController->UpdateAmmoUI(ClipCurrentAmmo, GunCurrentAmmo);
	}
}

void AFPSCharacterBase::ClientUpdateHealthUI_Implementation(float NewHealth, int CurMaxHealth)
{
	if(FPSPlayerController)
	{
		FPSPlayerController->UpdateHealthUI(NewHealth, CurMaxHealth);
	}
}

void AFPSCharacterBase::ClientRecoil_Implementation()
{
	UCurveFloat* VerticalRecoilCurve = nullptr;
	UCurveFloat* HorizontalRecoilCurve = nullptr;
	if(ClientPrimaryWeapon)
	{
		VerticalRecoilCurve = ClientPrimaryWeapon->VerticalRecoilCurve;
		HorizontalRecoilCurve = ClientPrimaryWeapon->HorizontalRecoilCurve;
	}
	
	RecoilXCoordPerFire += 0.1;
	if(VerticalRecoilCurve)
	{
		NewVerticalRecoilAmount = VerticalRecoilCurve->GetFloatValue(RecoilXCoordPerFire);
	}
	if(HorizontalRecoilCurve)
	{
		NewHorizontalRecoilAmount = HorizontalRecoilCurve->GetFloatValue(RecoilXCoordPerFire);
	}
	
	VerticalRecoilAmount = NewVerticalRecoilAmount - OldVerticalRecoilAmount;
	HorizontalRecoilAmount = NewHorizontalRecoilAmount - OldHorizontalRecoilAmount;
	
	if(FPSPlayerController)
	{
		FRotator ControllerRotator = FPSPlayerController->GetControlRotation();
		FPSPlayerController->SetControlRotation(FRotator(ControllerRotator.Pitch + VerticalRecoilAmount,
			ControllerRotator.Yaw + HorizontalRecoilAmount,
			ControllerRotator.Roll));
	}
	
	OldVerticalRecoilAmount = NewVerticalRecoilAmount;
	OldHorizontalRecoilAmount = NewHorizontalRecoilAmount;
}

void AFPSCharacterBase::ClientReload_Implementation()
{
	AWeaponBaseClient* CurrentClientWeapon = GetCurrentClientFPArmsWeaponActor();
	if(CurrentClientWeapon)
	{
		// 播放枪械动画
		CurrentClientWeapon->PlayReloadAnimation();
		
		// 播放手臂动画蒙太奇
		UAnimMontage* ClientArmsReloadMontage = CurrentClientWeapon->ClientArmsReloadAnimMontage;
		ClientArmsAnimBP->Montage_Play(ClientArmsReloadMontage);
	}
}

void AFPSCharacterBase::ClientOnDead_Implementation()
{
	DestroyWeaponOnDead();
}

void AFPSCharacterBase::AutomaticFire()
{
	if(ServerPrimaryWeapon->ClipCurrentAmmo > 0)
	{
		FireWeaponPrimaryImplementation();
	}
	else
	{
		StopFirePrimary(); 
	}
}

void AFPSCharacterBase::FireWeaponPrimary()
{
	if(ServerPrimaryWeapon->ClipCurrentAmmo > 0 && !IsReloading)
	{
		FireWeaponPrimaryImplementation();

		if(ServerPrimaryWeapon->IsAutomatic)
		{
			GetWorldTimerManager().SetTimer(AutomaticFireTimerHandle, this, &AFPSCharacterBase::AutomaticFire,
				ServerPrimaryWeapon->AutomaticFireRate, true);
		}
	}
}

void AFPSCharacterBase::FireWeaponPrimaryImplementation()
{
	if(ServerPrimaryWeapon->ClipCurrentAmmo > 0 && !IsReloading)
	{
		bool isMoving = true ? UKismetMathLibrary::VSize(GetVelocity()) > 0.1f : false;
		ServerFireRifleWeapon(PlayerCamera->GetComponentLocation(), PlayerCamera->GetComponentRotation(), isMoving);

		ClientFire();

		ClientRecoil();
	}
}

void AFPSCharacterBase::StopFirePrimary()
{
	ServerStopFiring();
	
	GetWorldTimerManager().ClearTimer(AutomaticFireTimerHandle);

	ResetRecoil();
}

void AFPSCharacterBase::RifleLineTrace(FVector CameraLocation, FRotator CameraRotation, bool IsMoving)
{
	FVector EndLocation;
	FVector CameraForwardVector = GetCameraForwardVector();
	TArray<AActor*> IgnoreArray;
	IgnoreArray.Add(this);
	FHitResult HitResult;
	
	if(ServerPrimaryWeapon)
	{
		if(IsMoving)
		{
			FVector Vector = CameraLocation + CameraForwardVector * ServerPrimaryWeapon->BulletDistance;
			float RandomX = UKismetMathLibrary::RandomFloatInRange(-ServerPrimaryWeapon->MovingFireRandomRange, ServerPrimaryWeapon->MovingFireRandomRange);
			float RandomY = UKismetMathLibrary::RandomFloatInRange(-ServerPrimaryWeapon->MovingFireRandomRange, ServerPrimaryWeapon->MovingFireRandomRange);
			float RandomZ = UKismetMathLibrary::RandomFloatInRange(-ServerPrimaryWeapon->MovingFireRandomRange, ServerPrimaryWeapon->MovingFireRandomRange);
			EndLocation = FVector(Vector.X + RandomX, Vector.Y + RandomY, Vector.Z + RandomZ);
		}
		else
		{
			EndLocation = CameraLocation + CameraForwardVector * ServerPrimaryWeapon->BulletDistance;
		}
	
		bool HitSuccess = UKismetSystemLibrary::LineTraceSingle(GetWorld(), CameraLocation, EndLocation, ETraceTypeQuery::TraceTypeQuery1, false,
			IgnoreArray, EDrawDebugTrace::None, HitResult, true, FLinearColor::Red,
			FLinearColor::Green, 3.f);

		if(HitSuccess)
		{
			// UKismetSystemLibrary::PrintString(GetWorld(), FString::Printf(TEXT("Hit Actor Name : %s"), *HitResult.Actor->GetName()));
			AFPSCharacterBase* FPSCharacterBase = Cast<AFPSCharacterBase>(HitResult.Actor);
			if(FPSCharacterBase)
			{
				// 打到玩家
				DamagePlayer(HitResult.Actor.Get(), CameraLocation, HitResult);
			}
			else
			{
				// 打到物体
				FRotator XRotator = UKismetMathLibrary::MakeRotFromX(HitResult.Normal);
				MultiSpawnBulletHole(HitResult.Location, XRotator);
			}
		}
	}
}

void AFPSCharacterBase::FireWeaponSecondary()
{
	if(ServerSecondaryWeapon->ClipCurrentAmmo > 0 && !IsReloading)
	{
		bool isMoving = true ? UKismetMathLibrary::VSize(GetVelocity()) > 0.1f : false;
		ServerFirePistolWeapon(PlayerCamera->GetComponentLocation(), PlayerCamera->GetComponentRotation(), isMoving);

		ClientFire();
	}
}

void AFPSCharacterBase::StopFireSecondary()
{
	ServerStopFiring();
}

void AFPSCharacterBase::PistolLineTrace(FVector CameraLocation, FRotator CameraRotation, bool IsMoving)
{
	FVector EndLocation;
	FVector CameraForwardVector;
	TArray<AActor*> IgnoreArray;
	IgnoreArray.Add(this);
	FHitResult HitResult;
	
	if(ServerSecondaryWeapon)
	{
		if(IsMoving)
		{
			CameraForwardVector = GetCameraForwardVector();
			FVector Vector = CameraLocation + CameraForwardVector * ServerSecondaryWeapon->BulletDistance;
			float RandomX = UKismetMathLibrary::RandomFloatInRange(-ServerSecondaryWeapon->MovingFireRandomRange, ServerSecondaryWeapon->MovingFireRandomRange);
			float RandomY = UKismetMathLibrary::RandomFloatInRange(-ServerSecondaryWeapon->MovingFireRandomRange, ServerSecondaryWeapon->MovingFireRandomRange);
			float RandomZ = UKismetMathLibrary::RandomFloatInRange(-ServerSecondaryWeapon->MovingFireRandomRange, ServerSecondaryWeapon->MovingFireRandomRange);
			EndLocation = FVector(Vector.X + RandomX, Vector.Y + RandomY, Vector.Z + RandomZ);
		}
		else
		{
			// 打得越快，偏移越大
			FRotator Rotator = CalPistolSpreadRotation(CameraRotation);
			CameraForwardVector = UKismetMathLibrary::GetForwardVector(Rotator);
			EndLocation = CameraLocation + CameraForwardVector * ServerSecondaryWeapon->BulletDistance;
		}
	
		bool HitSuccess = UKismetSystemLibrary::LineTraceSingle(GetWorld(), CameraLocation, EndLocation, ETraceTypeQuery::TraceTypeQuery1, false,
			IgnoreArray, EDrawDebugTrace::None, HitResult, true, FLinearColor::Red,
			FLinearColor::Green, 3.f);
		
		PistolSpreadMin -= ServerSecondaryWeapon->SpreadMinStep;
		PistolSpreadMax += ServerSecondaryWeapon->SpreadMaxStep;
		
		if(HitSuccess)
		{
			AFPSCharacterBase* FPSCharacterBase = Cast<AFPSCharacterBase>(HitResult.Actor);
			if(FPSCharacterBase)
			{
				// 打到玩家
				DamagePlayer(HitResult.Actor.Get(), CameraLocation, HitResult);
			}
			else
			{
				// 打到物体
				FRotator XRotator = UKismetMathLibrary::MakeRotFromX(HitResult.Normal);
				MultiSpawnBulletHole(HitResult.Location, XRotator);
			}
		}
	}
}

FRotator AFPSCharacterBase::CalPistolSpreadRotation(FRotator CameraRotation)
{
	FRotator Rotator;
	Rotator.Roll = CameraRotation.Roll;
	Rotator.Pitch = CameraRotation.Pitch + UKismetMathLibrary::RandomFloatInRange(PistolSpreadMin, PistolSpreadMax);
	Rotator.Yaw = CameraRotation.Yaw + UKismetMathLibrary::RandomFloatInRange(PistolSpreadMin, PistolSpreadMax);
	return Rotator;
}

void AFPSCharacterBase::DelayResetPistolSpreadCallBack()
{
	PistolSpreadMin = 0;
	PistolSpreadMax = 0;
}

void AFPSCharacterBase::ResetRecoil()
{
	NewVerticalRecoilAmount = 0;
	OldVerticalRecoilAmount = 0;
	VerticalRecoilAmount = 0;
	NewHorizontalRecoilAmount = 0;
	OldHorizontalRecoilAmount = 0;
	HorizontalRecoilAmount = 0;
	RecoilXCoordPerFire = 0;
}

void AFPSCharacterBase::DelayPlayArmReloadCallBack()
{
	AWeaponBaseServer* CurrentServerWeapon = GetCurrentServerTPBodiesWeaponActor();
	if(CurrentServerWeapon)
	{
		IsReloading = false;
		int32 GunCurrentAmmo = CurrentServerWeapon->GunCurrentAmmo;
		int32 ClipCurrentAmmo = CurrentServerWeapon->ClipCurrentAmmo;
		int32 const MaxClipAmmo = CurrentServerWeapon->MaxClipAmmo;

		if(MaxClipAmmo - ClipCurrentAmmo >= GunCurrentAmmo)
		{
			ClipCurrentAmmo += GunCurrentAmmo;
			GunCurrentAmmo = 0;
		}
		else
		{
			GunCurrentAmmo -= MaxClipAmmo - ClipCurrentAmmo;
			ClipCurrentAmmo = MaxClipAmmo;
		}
		CurrentServerWeapon->GunCurrentAmmo = GunCurrentAmmo;
		CurrentServerWeapon->ClipCurrentAmmo = ClipCurrentAmmo;
		ClientUpdateAmmoUI(ClipCurrentAmmo, GunCurrentAmmo);
	}
}

void AFPSCharacterBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	// 父类也实现了该方法，因此需调用下父类的方法
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(AFPSCharacterBase, IsFiring, COND_None);
	DOREPLIFETIME_CONDITION(AFPSCharacterBase, IsReloading, COND_None);
	DOREPLIFETIME_CONDITION(AFPSCharacterBase, ServerHoldWeaponKind, COND_None);
}

void AFPSCharacterBase::DamagePlayer(AActor* DamagedActor, FVector& HitFromDirection, FHitResult& HitInfo)
{
	AWeaponBaseServer* CurrentServerWeapon = GetCurrentServerTPBodiesWeaponActor();
	if(CurrentServerWeapon)
	{
		float Damage = CurrentServerWeapon->BaseDamage;
		// 通过判断击中的物理材质来判断击中的部位，会比添加额外胶囊体的方法更加精准
		switch (HitInfo.PhysMaterial->SurfaceType)
		{
		case SurfaceType1:
			{
				// Head
				Damage *= HeadDamageRatio;
			}
			break;
		case SurfaceType2:
			{
				// Body
				Damage *= BodyDamageRatio;
			}
			break;
		case SurfaceType3:
			{
				// Arm
				Damage *= ArmDamageRatio;
			}
			break;
		case SurfaceType4:
			{
				// Leg
				Damage *= LegDamageRatio;
			}
			break;
		}
		UGameplayStatics::ApplyPointDamage(DamagedActor, Damage, HitFromDirection, HitInfo,
			GetController(), this, UDamageType::StaticClass());
	}
}

void AFPSCharacterBase::DamagePlayerBySkill(AActor* DamagedActor, float BaseDamage)
{
	UGameplayStatics::ApplyDamage(DamagedActor, BaseDamage, GetController(), this, UDamageType::StaticClass());
}

void AFPSCharacterBase::OnHit(AActor* DamagedActor, float Damage, AController* InstigatedBy, FVector HitLocation, UPrimitiveComponent* FHitComponent, FName BoneName, FVector ShotFromDirection, const UDamageType* DamageType, AActor* DamageCauser)
{
	if(!IsDead())
	{
		Health -= Damage;
		ClientUpdateHealthUI(Health, MaxHealth);
		if(Health <= 0)
		{
			OnDead(DamageCauser);
		}
	}
}

void AFPSCharacterBase::OnHitBySkill(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
	AController* InstigatedBy, AActor* DamageCauser)
{
	if(!IsDead())
	{
		Health -= Damage;
		ClientUpdateHealthUI(Health, MaxHealth);
		if(Health <= 0)
		{
			OnDead(DamageCauser);
		}
	}
}

void AFPSCharacterBase::OnDead(AActor* DamageCauser)
{
	DestroyWeaponOnDead();
	ClientOnDead();
	AMultiFPSPlayerController* MultiFPSPlayerController = Cast<AMultiFPSPlayerController>(GetController());
	if(MultiFPSPlayerController)
	{
		MultiFPSPlayerController->OnDead(DamageCauser);
	}
}

bool AFPSCharacterBase::IsDead()
{
	return Health <= 0;
}

void AFPSCharacterBase::DestroyWeaponOnDead()
{
	if(ServerPrimaryWeapon)
	{
		ServerPrimaryWeapon->Destroy();
	}
	if(ServerSecondaryWeapon)
	{
		ServerSecondaryWeapon->Destroy();
	}
	if(ClientPrimaryWeapon)
	{
		ClientPrimaryWeapon->Destroy();
	}
	if(ClientSecondaryWeapon)
	{
		ClientSecondaryWeapon->Destroy();
	}
}

void AFPSCharacterBase::SwitchPrimary()
{
	ServerSwitchPrimary();
}

void AFPSCharacterBase::SwitchSecondary()
{
	ServerSwitchSecondary();
}

FVector AFPSCharacterBase::GetCameraLocation()
{
	FVector Location = PlayerCamera->GetComponentLocation();
	return Location;
}

FRotator AFPSCharacterBase::GetCameraRotation()
{
	FRotator Rotation = PlayerCamera->GetComponentRotation();
	if(!IsLocallyControlled())
	{
		// RemoteViewPitch是客户端同步给服务端的Pitch值
		Rotation.Pitch = RemoteViewPitch * 360 / 255;
	}
	return Rotation;
}

FTransform AFPSCharacterBase::GetCameraTransform()
{
	FVector Location = PlayerCamera->GetComponentLocation();
	FRotator Rotation = PlayerCamera->GetComponentRotation();
	if(!IsLocallyControlled())
	{
		// RemoteViewPitch是客户端同步给服务端的Pitch值
		Rotation.Pitch = RemoteViewPitch * 360 / 255;
	}
	return FTransform(Rotation, Location);
}

FVector AFPSCharacterBase::GetCameraForwardVector()
{
	return UKismetMathLibrary::GetForwardVector(GetCameraRotation());
}

void AFPSCharacterBase::ClientEquipFPArmsPrimary_Implementation()
{
	if(IsLocallyControlled())
	{
		if(ServerPrimaryWeapon)
		{
			FActorSpawnParameters SpawnInfo;
			SpawnInfo.Owner = this;
			SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			ClientPrimaryWeapon = GetWorld()->SpawnActor<AWeaponBaseClient>(ServerPrimaryWeapon->ClientWeaponBaseBPClass,
				GetActorTransform(),
				SpawnInfo);
			ClientPrimaryWeapon->K2_AttachToComponent(FPArmsMesh, ClientPrimaryWeapon->ClientArmsWeaponSocketName,
				EAttachmentRule::SnapToTarget,
				EAttachmentRule::SnapToTarget,
				EAttachmentRule::SnapToTarget,
				true);
			if(ServerHoldWeaponKind == EWeaponKind::Primary)
			{
				ClientUpdateAmmoUI(ServerPrimaryWeapon->ClipCurrentAmmo, ServerPrimaryWeapon->GunCurrentAmmo);
				ClientHoldWeapon = ClientPrimaryWeapon;
				UpdateFPArmsBlendPose(ClientPrimaryWeapon->FPArmsBlendPose);
			}
			else
			{
				ClientPrimaryWeapon->SetActorHiddenInGame(true);
			}
		}
	}
}

void AFPSCharacterBase::ClientEquipFPArmsSecondary_Implementation()
{
	if(IsLocallyControlled())
	{
		if(ServerSecondaryWeapon)
		{
			FActorSpawnParameters SpawnInfo;
			SpawnInfo.Owner = this;
			SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			ClientSecondaryWeapon = GetWorld()->SpawnActor<AWeaponBaseClient>(ServerSecondaryWeapon->ClientWeaponBaseBPClass,
				GetActorTransform(),
				SpawnInfo);
			ClientSecondaryWeapon->K2_AttachToComponent(FPArmsMesh, ClientSecondaryWeapon->ClientArmsWeaponSocketName,
				EAttachmentRule::SnapToTarget,
				EAttachmentRule::SnapToTarget,
				EAttachmentRule::SnapToTarget,
				true);
			if(ServerHoldWeaponKind == EWeaponKind::Secondary)
			{
				ClientUpdateAmmoUI(ServerSecondaryWeapon->ClipCurrentAmmo, ServerSecondaryWeapon->GunCurrentAmmo);
				ClientHoldWeapon = ClientSecondaryWeapon;
				UpdateFPArmsBlendPose(ClientSecondaryWeapon->FPArmsBlendPose);	
			}
			else
			{
				ClientSecondaryWeapon->SetActorHiddenInGame(true);
			}
		}
	}
}

void AFPSCharacterBase::MoveForward(float AxisValue)
{
	AddMovementInput(GetActorForwardVector(), AxisValue, false);
}

void AFPSCharacterBase::MoveRight(float AxisValue)
{
	AddMovementInput(GetActorRightVector(), AxisValue, false);
}

void AFPSCharacterBase::LowSpeedWalkAction()
{
	CharacterMovementComponent->MaxWalkSpeed = 300;
	ServerLowSpeedWalkAction();
}

void AFPSCharacterBase::NormalSpeedWalkAction()
{
	CharacterMovementComponent->MaxWalkSpeed = 600;
	ServerNormalSpeedWalkAction();
}

void AFPSCharacterBase::JumpAction()
{
	Jump();
}

void AFPSCharacterBase::StopJumpAction()
{
	StopJumping();
}

void AFPSCharacterBase::InputFirePressed()
{
	if(ServerHoldWeapon->KindOfWeapon == EWeaponKind::Primary)
	{
		FireWeaponPrimary();
	}
	else if(ServerHoldWeapon->KindOfWeapon == EWeaponKind::Secondary)
	{
		FireWeaponSecondary();
	}
}

void AFPSCharacterBase::InputFireReleased()
{
	if(ServerHoldWeapon->KindOfWeapon == EWeaponKind::Primary)
	{
		StopFirePrimary();
	}
	else if(ServerHoldWeapon->KindOfWeapon == EWeaponKind::Secondary)
	{
		StopFireSecondary();
	}
}

void AFPSCharacterBase::InputReload()
{
	if(!IsReloading)
	{
		if(!IsFiring)
		{
			if(ServerHoldWeapon->KindOfWeapon == EWeaponKind::Primary)
			{
				ServerReloadPrimary();
			}
			else if(ServerHoldWeapon->KindOfWeapon == EWeaponKind::Secondary)
			{
				ServerReloadSecondary();
			}
		}
	}
}

void AFPSCharacterBase::InputSwitchPrimary()
{
	if(!IsFiring && !IsReloading)
	{
		SwitchPrimary();
	}
}

void AFPSCharacterBase::InputSwitchSecondary()
{
	if(!IsFiring && !IsReloading)
	{
		SwitchSecondary();
	}
}

void AFPSCharacterBase::BindAbilitySystemComponentInput()
{
	if(AbilitySystemComponent && IsValid(InputComponent))
	{
		AbilitySystemComponent->BindAbilityActivationToInputComponent(
			InputComponent,
			FGameplayAbilityInputBinds(
				FString("ConfirmTarget"),
				FString("CancelTarget"),
				FString("EAbilityInputID")
			));
	}
}

void AFPSCharacterBase::EquipPrimary(AWeaponBaseServer* WeaponBaseServer)
{
	if(!ServerPrimaryWeapon)
	{
		ServerPrimaryWeapon = WeaponBaseServer;
		ServerPrimaryWeapon->SetOwner(this);
		if(!ServerHoldWeapon && (GetWorld()->IsServer() || ServerHoldWeaponKind == EWeaponKind::Primary))
		{
			if(GetWorld()->IsServer())
			{
				ServerHoldWeaponKind = EWeaponKind::Primary;
			}
			ServerHoldWeapon = ServerPrimaryWeapon;
			ServerPrimaryWeapon->K2_AttachToComponent(TPBodiesMesh, TEXT("Weapon_Rifle"),
				EAttachmentRule::SnapToTarget,
				EAttachmentRule::SnapToTarget,
				EAttachmentRule::SnapToTarget,
				true);
		}
		else
		{
			ServerPrimaryWeapon->K2_AttachToComponent(TPBodiesMesh, TEXT("Backup_Rifle"),
				EAttachmentRule::SnapToTarget,
				EAttachmentRule::SnapToTarget,
				EAttachmentRule::SnapToTarget,
				true);
		}
		ClientEquipFPArmsPrimary();
	}
}

void AFPSCharacterBase::EquipSecondary(AWeaponBaseServer* WeaponBaseServer)
{
	if(!ServerSecondaryWeapon)
	{
		ServerSecondaryWeapon = WeaponBaseServer;
		ServerSecondaryWeapon->SetOwner(this);
		if(!ServerHoldWeapon && (GetWorld()->IsServer() || ServerHoldWeaponKind == EWeaponKind::Secondary))
		{
			if(GetWorld()->IsServer())
			{
				ServerHoldWeaponKind = EWeaponKind::Secondary;
			}
			ServerHoldWeapon = ServerSecondaryWeapon;
			ServerSecondaryWeapon->K2_AttachToComponent(TPBodiesMesh, TEXT("Weapon_Rifle"),
				EAttachmentRule::SnapToTarget,
				EAttachmentRule::SnapToTarget,
				EAttachmentRule::SnapToTarget,
				true);
		}
		else
		{
			ServerSecondaryWeapon->K2_AttachToComponent(TPBodiesMesh, TEXT("Backup_Pistol"),
				EAttachmentRule::SnapToTarget,
				EAttachmentRule::SnapToTarget,
				EAttachmentRule::SnapToTarget,
				true);
		}
		ClientEquipFPArmsSecondary();
	}
}

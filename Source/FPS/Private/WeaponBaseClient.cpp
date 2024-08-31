// Fill out your copyright notice in the Description page of Project Settings.


#include "WeaponBaseClient.h"

#include "Kismet/GameplayStatics.h"

// Sets default values
AWeaponBaseClient::AWeaponBaseClient()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponBase"));
	RootComponent = WeaponMesh;
	WeaponMesh->SetOnlyOwnerSee(true);
}

void AWeaponBaseClient::DisplayWeaponEffect()
{
	UGameplayStatics::SpawnEmitterAttached(MuzzleFlash, WeaponMesh, TEXT("Fire_FX_Slot"),
		FVector::ZeroVector, FRotator::ZeroRotator, FVector::OneVector,
		EAttachLocation::KeepRelativeOffset, true, EPSCPoolMethod::None,
		true);
	// 自己听到的声音大小跟距离无关，所以是2D
	UGameplayStatics::PlaySound2D(GetWorld(), FireSound);
}

// Called when the game starts or when spawned
void AWeaponBaseClient::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AWeaponBaseClient::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}


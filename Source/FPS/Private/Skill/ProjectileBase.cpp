// Fill out your copyright notice in the Description page of Project Settings.


#include "Skill/ProjectileBase.h"

#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"

// Sets default values
AProjectileBase::AProjectileBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	SphereCollision = CreateDefaultSubobject<USphereComponent>(TEXT("SphereCollision"));
	Arrow = CreateDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
	ParticleSystem = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("ParticleSystem"));
	
	Arrow->SetupAttachment(SphereCollision);
	ParticleSystem->SetupAttachment(SphereCollision);
	
	SphereCollision->OnComponentHit.AddDynamic(this, &AProjectileBase::OnHit);
	SphereCollision->OnComponentBeginOverlap.AddDynamic(this, &AProjectileBase::OnOtherOverlap);

	bReplicates = true;
	
}

// Called when the game starts or when spawned
void AProjectileBase::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AProjectileBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
}

void AProjectileBase::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), HitEmitterTemplate, Hit.Location);
	Destroy();
}

void AProjectileBase::OnOtherOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AFPSCharacterBase* OtherFPSCharacter = Cast<AFPSCharacterBase>(OtherActor);
	AFPSCharacterBase* TheInstigator = Cast<AFPSCharacterBase>(GetInstigator());
	if(OtherFPSCharacter && TheInstigator && OtherFPSCharacter != TheInstigator)
	{
		TheInstigator->DamagePlayerBySkill(OtherFPSCharacter, BaseDamage);
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), HitEmitterTemplate, SweepResult.Location);
		Destroy();
	}
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "Skill/MagicFieldBase.h"

#include "FPSCharacterBase.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"

// Sets default values
AMagicFieldBase::AMagicFieldBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	CapsuleCollision = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleCollision "));
	ParticleSystem = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("ParticleSystem"));

	ParticleSystem->SetupAttachment(CapsuleCollision);

	CapsuleCollision->OnComponentBeginOverlap.AddDynamic(this, &AMagicFieldBase::OnOtherBeginOverlap);
	CapsuleCollision->OnComponentEndOverlap.AddDynamic(this, &AMagicFieldBase::OnOtherEndOverlap);
	
	bReplicates = true;
	
}

// Called when the game starts or when spawned
void AMagicFieldBase::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AMagicFieldBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	for(int i = 0; i < HitActors.Num(); ++i)
	{
		AActor* Actor = HitActors[i];
		if(Actor)
		{
			UGameplayStatics::ApplyDamage(Actor, BaseDamage, GetInstigator()->GetController(), this, UDamageType::StaticClass());
		}
	}
}

void AMagicFieldBase::OnOtherBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AFPSCharacterBase* FpsCharacter = Cast<AFPSCharacterBase>(OtherActor);
	if(FpsCharacter && FpsCharacter != GetInstigator() && !HitActors.Contains(FpsCharacter))
	{
		HitActors.Add(FpsCharacter);
	}
}

void AMagicFieldBase::OnOtherEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	HitActors.Remove(OtherActor);
}

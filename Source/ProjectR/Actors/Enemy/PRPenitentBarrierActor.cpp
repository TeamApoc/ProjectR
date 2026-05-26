// Copyright ProjectR. All Rights Reserved.

#include "PRPenitentBarrierActor.h"

#include "AbilitySystemComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Common.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/Character/Enemy/Penitent/PRPenitentCharacter.h"
#include "ProjectR/Combat/PRCombatStatics.h"
#include "ProjectR/PRGameplayTags.h"

APRPenitentBarrierActor::APRPenitentBarrierActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	SetRootComponent(SphereComponent);
	SphereComponent->SetSphereRadius(80.0f);
	SphereComponent->SetCollisionProfileName(AttachedCollisionProfileName);
	SphereComponent->OnComponentHit.AddDynamic(this, &APRPenitentBarrierActor::HandleBarrierHit);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(SphereComponent);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->UpdatedComponent = SphereComponent;
	ProjectileMovementComponent->InitialSpeed = LaunchSpeed;
	ProjectileMovementComponent->MaxSpeed = LaunchSpeed;
	ProjectileMovementComponent->bAutoActivate = false;

	AbilitySystemComponent = CreateDefaultSubobject<UPRAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	CommonSet = CreateDefaultSubobject<UPRAttributeSet_Common>(TEXT("CommonSet"));
}

void APRPenitentBarrierActor::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
		InitializeBarrierHealth();
	}
}

void APRPenitentBarrierActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(OwningPenitent))
	{
		const bool bOwnerStillReferencesThis = OwningPenitent->GetSpawnedBarrierActor() == this;
		if (bOwnerStillReferencesThis)
		{
			if (UPRAbilitySystemComponent* PenitentAbilitySystemComponent = OwningPenitent->GetEnemyAbilitySystemComponent())
			{
				if (PenitentAbilitySystemComponent->HasMatchingGameplayTag(PRGameplayTags::State_Enemy_Penitent_BarrierSummon))
				{
					// 배리어 보유 상태 태그 정리
					PenitentAbilitySystemComponent->RemoveLooseGameplayTag(PRGameplayTags::State_Enemy_Penitent_BarrierSummon);
				}
			}

			OwningPenitent->ClearSpawnedBarrierActor(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

UAbilitySystemComponent* APRPenitentBarrierActor::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void APRPenitentBarrierActor::OnPostDamageApplied(const FPRDamageAppliedContext& Context)
{
	if (Context.HealthAfterDamage <= 0.0f)
	{
		DestroyBarrier();
	}
}

void APRPenitentBarrierActor::InitializeAttachedBarrier(APRPenitentCharacter* OwnerPenitent)
{
	OwningPenitent = OwnerPenitent;
	bDamageEnabled = false;

	if (IsValid(SphereComponent))
	{
		SphereComponent->SetCollisionProfileName(AttachedCollisionProfileName);
	}

	if (IsValid(ProjectileMovementComponent))
	{
		ProjectileMovementComponent->StopMovementImmediately();
		ProjectileMovementComponent->Deactivate();
	}
}

void APRPenitentBarrierActor::SetBarrierDamageEffectSpec(const FGameplayEffectSpecHandle& InEffectSpecHandle)
{
	DamageEffectSpecHandle = InEffectSpecHandle;
}

void APRPenitentBarrierActor::FireBarrier(const FVector& LaunchDirection)
{
	if (!HasAuthority())
	{
		return;
	}

	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	bDamageEnabled = true;

	if (IsValid(SphereComponent))
	{
		SphereComponent->SetCollisionProfileName(FiredCollisionProfileName);
	}

	const FVector SafeLaunchDirection = LaunchDirection.GetSafeNormal();
	if (IsValid(ProjectileMovementComponent) && !SafeLaunchDirection.IsNearlyZero())
	{
		ProjectileMovementComponent->InitialSpeed = LaunchSpeed;
		ProjectileMovementComponent->MaxSpeed = FMath::Max(ProjectileMovementComponent->MaxSpeed, LaunchSpeed);
		ProjectileMovementComponent->Velocity = SafeLaunchDirection * LaunchSpeed;
		ProjectileMovementComponent->Activate(true);
		SetActorRotation(SafeLaunchDirection.Rotation());
	}

	if (FiredLifeSpan > 0.0f)
	{
		SetLifeSpan(FiredLifeSpan);
	}
}

void APRPenitentBarrierActor::DestroyBarrier()
{
	if (IsValid(OwningPenitent))
	{
		const bool bOwnerStillReferencesThis = OwningPenitent->GetSpawnedBarrierActor() == this;
		if (bOwnerStillReferencesThis)
		{
			if (UPRAbilitySystemComponent* PenitentAbilitySystemComponent = OwningPenitent->GetEnemyAbilitySystemComponent())
			{
				if (PenitentAbilitySystemComponent->HasMatchingGameplayTag(PRGameplayTags::State_Enemy_Penitent_BarrierSummon))
				{
					// 배리어 보유 상태 태그 정리
					PenitentAbilitySystemComponent->RemoveLooseGameplayTag(PRGameplayTags::State_Enemy_Penitent_BarrierSummon);
				}
			}
		}

		OwningPenitent->ClearSpawnedBarrierActor(this);
	}

	Destroy();
}

void APRPenitentBarrierActor::HandleBarrierHit(UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	FVector NormalImpulse,
	const FHitResult& Hit)
{
	if (!HasAuthority() || !bDamageEnabled || !IsValid(OtherActor) || OtherActor == this || OtherActor == OwningPenitent)
	{
		return;
	}

	ApplyBarrierDamage(OtherActor, Hit);
	DestroyBarrier();
}

void APRPenitentBarrierActor::ApplyBarrierDamage(AActor* TargetActor, const FHitResult& Hit)
{
	if (!IsValid(TargetActor) || UPRCombatStatics::GetActorTeam(TargetActor) == EPRTeam::Enemy)
	{
		return;
	}

	UAbilitySystemComponent* TargetAbilitySystemComponent = UPRCombatStatics::FindAbilitySystemComponent(TargetActor);
	if (!IsValid(TargetAbilitySystemComponent))
	{
		return;
	}

	if (FGameplayEffectSpec* EffectSpec = DamageEffectSpecHandle.Data.Get())
	{
		EffectSpec->GetContext().AddHitResult(Hit, true);
		TargetAbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*EffectSpec);
	}
}

void APRPenitentBarrierActor::InitializeBarrierHealth()
{
	if (!IsValid(CommonSet))
	{
		return;
	}

	CommonSet->SetMaxHealth(BarrierHealth);
	CommonSet->SetHealth(BarrierHealth);
}

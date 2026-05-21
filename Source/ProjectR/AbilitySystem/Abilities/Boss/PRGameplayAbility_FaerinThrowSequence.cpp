// Copyright ProjectR. All Rights Reserved.

#include "PRGameplayAbility_FaerinThrowSequence.h"

#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameplayEffect.h"
#include "TimerManager.h"
#include "ProjectR/AbilitySystem/Abilities/Boss/PRBossAbilityTagUtils.h"
#include "ProjectR/AbilitySystem/Data/PRAbilitySystemRegistry.h"
#include "ProjectR/Combat/PRCombatGameplayTags.h"
#include "ProjectR/Projectile/PRProjectileBase.h"
#include "ProjectR/System/PRAssetManager.h"
#include "ProjectR/PRGameplayTags.h"

UPRGameplayAbility_FaerinThrowSequence::UPRGameplayAbility_FaerinThrowSequence()
{
	SetAssetTags(PRBossAbility::MakePatternAssetTags(PRGameplayTags::Ability_Boss_Faerin_ThrowSequence));
}

void UPRGameplayAbility_FaerinThrowSequence::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	bThrowProjectileSpawned = false;

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UPRGameplayAbility_FaerinThrowSequence::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const bool bReplicateEndAbility,
	const bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ThrowReleaseTimerHandle);
	}

	bThrowProjectileSpawned = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UPRGameplayAbility_FaerinThrowSequence::NativeOnStageStarted(const FPRFaerinStagedMontageStage& Stage)
{
	if (!ThrowStageName.IsNone() && Stage.StageName != ThrowStageName)
	{
		return;
	}

	ScheduleThrowProjectile();
}

void UPRGameplayAbility_FaerinThrowSequence::ScheduleThrowProjectile()
{
	UWorld* World = GetWorld();
	if (!IsValid(World) || bThrowProjectileSpawned)
	{
		return;
	}

	FTimerManager& TimerManager = World->GetTimerManager();
	if (TimerManager.IsTimerActive(ThrowReleaseTimerHandle))
	{
		return;
	}

	if (ThrowReleaseDelay <= 0.0f)
	{
		SpawnThrowProjectile();
		return;
	}

	TimerManager.SetTimer(
		ThrowReleaseTimerHandle,
		this,
		&UPRGameplayAbility_FaerinThrowSequence::SpawnThrowProjectile,
		ThrowReleaseDelay,
		false);
}

void UPRGameplayAbility_FaerinThrowSequence::SpawnThrowProjectile()
{
	if (bThrowProjectileSpawned || !IsValid(ThrowProjectileClass))
	{
		return;
	}

	FTransform SpawnTransform;
	if (!BuildThrowProjectileSpawnTransform(SpawnTransform))
	{
		return;
	}

	UWorld* World = GetWorld();
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(World) || !IsValid(AvatarActor) || !AvatarActor->HasAuthority())
	{
		return;
	}

	const uint32 ProjectileId = NextThrowProjectileId++;
	if (NextThrowProjectileId == 0)
	{
		NextThrowProjectileId = 1;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = AvatarActor;
	SpawnParameters.Instigator = Cast<APawn>(AvatarActor);
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParameters.CustomPreSpawnInitalization = [ProjectileId](AActor* Actor)
	{
		if (APRProjectileBase* Projectile = Cast<APRProjectileBase>(Actor))
		{
			Projectile->InitializeProjectile(EPRProjectileRole::Auth, ProjectileId);
		}
	};

	APRProjectileBase* SpawnedProjectile = World->SpawnActor<APRProjectileBase>(
		ThrowProjectileClass,
		SpawnTransform,
		SpawnParameters);

	if (!IsValid(SpawnedProjectile))
	{
		return;
	}

	const FVector AimDirection = CalculateThrowAimDirection(SpawnTransform.GetLocation());
	SpawnedProjectile->SetProjectileInitialVelocity(AimDirection, ProjectileSpeedOverride);

	if (bUseTrackingProjectile)
	{
		if (const AActor* TargetActor = GetBossPatternTarget())
		{
			SpawnedProjectile->ConfigureProjectileHoming(TargetActor->GetRootComponent(), ProjectileHomingAcceleration);
		}
	}

	const FGameplayEffectSpecHandle EffectSpecHandle = BuildThrowProjectileEffectSpec();
	if (EffectSpecHandle.IsValid())
	{
		SpawnedProjectile->InitGameplayEffectSpec(EffectSpecHandle);
	}

	SpawnedProjectile->PushRepMovement(EPRRepMovementEvent::Spawn);
	bThrowProjectileSpawned = true;
}

bool UPRGameplayAbility_FaerinThrowSequence::BuildThrowProjectileSpawnTransform(FTransform& OutTransform) const
{
	const FVector SpawnLocation = ResolveThrowSpawnLocation();
	if (SpawnLocation.IsNearlyZero())
	{
		return false;
	}

	const FVector AimDirection = CalculateThrowAimDirection(SpawnLocation);
	if (AimDirection.IsNearlyZero())
	{
		return false;
	}

	OutTransform = FTransform(AimDirection.Rotation() + ProjectileRotationOffset, SpawnLocation);
	return true;
}

FVector UPRGameplayAbility_FaerinThrowSequence::CalculateThrowAimDirection(const FVector& SpawnLocation) const
{
	const AActor* TargetActor = GetBossPatternTarget();
	if (!IsValid(TargetActor))
	{
		const AActor* AvatarActor = GetAvatarActorFromActorInfo();
		return IsValid(AvatarActor) ? AvatarActor->GetActorForwardVector() : FVector::ForwardVector;
	}

	FVector AimLocation = TargetActor->GetActorLocation();
	const float ResolvedSpeed = ProjectileSpeedOverride > 0.0f ? ProjectileSpeedOverride : 3500.0f;
	const float Distance = FVector::Distance(SpawnLocation, AimLocation);
	if (ResolvedSpeed > 0.0f && ProjectileTargetLead > 0.0f)
	{
		const float TravelTime = Distance / ResolvedSpeed;
		const float LeadScale = ProjectileTargetLead * 0.01f;
		AimLocation += TargetActor->GetVelocity() * TravelTime * LeadScale;
	}

	return (AimLocation - SpawnLocation).GetSafeNormal();
}

FGameplayEffectSpecHandle UPRGameplayAbility_FaerinThrowSequence::BuildThrowProjectileEffectSpec() const
{
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(SourceASC))
	{
		return FGameplayEffectSpecHandle();
	}

	TSubclassOf<UGameplayEffect> ResolvedDamageEffectClass = ProjectileDamageEffectClass;
	if (!IsValid(ResolvedDamageEffectClass))
	{
		const UPRAbilitySystemRegistry* Registry = UPRAssetManager::Get().GetAbilitySystemRegistry();
		if (IsValid(Registry))
		{
			ResolvedDamageEffectClass = Registry->DamageGE_FromEnemy;
		}
	}

	if (!IsValid(ResolvedDamageEffectClass))
	{
		return FGameplayEffectSpecHandle();
	}

	FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
	EffectContext.AddSourceObject(const_cast<UPRGameplayAbility_FaerinThrowSequence*>(this));

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(
		ResolvedDamageEffectClass,
		1.0f,
		EffectContext);

	if (SpecHandle.IsValid())
	{
		SpecHandle.Data->SetSetByCallerMagnitude(
			PRCombatGameplayTags::SetByCaller_AttackMultiplier,
			ProjectileDamageMultiplier);

		SpecHandle.Data->SetSetByCallerMagnitude(
			PRCombatGameplayTags::SetByCaller_PoiseDamage,
			ProjectilePoiseDamage);
	}

	return SpecHandle;
}

FVector UPRGameplayAbility_FaerinThrowSequence::ResolveThrowSpawnLocation() const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor))
	{
		return FVector::ZeroVector;
	}

	const ACharacter* Character = Cast<ACharacter>(AvatarActor);
	if (IsValid(Character))
	{
		const USkeletalMeshComponent* MeshComponent = Character->GetMesh();
		if (IsValid(MeshComponent)
			&& !ProjectileSpawnSocketName.IsNone()
			&& MeshComponent->DoesSocketExist(ProjectileSpawnSocketName))
		{
			return MeshComponent->GetSocketLocation(ProjectileSpawnSocketName);
		}
	}

	return AvatarActor->GetActorTransform().TransformPositionNoScale(ProjectileSpawnLocalOffset);
}

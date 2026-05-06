// Copyright ProjectR. All Rights Reserved.

#include "PRBossPortalActor.h"

#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "ProjectR/AbilitySystem/Data/PRAbilitySystemRegistry.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/Character/Enemy/PRBossBaseCharacter.h"
#include "ProjectR/Combat/PRCombatGameplayTags.h"
#include "ProjectR/Projectile/PRBossProjectileActor.h"
#include "ProjectR/Projectile/PRProjectileBase.h"
#include "ProjectR/System/PRAssetManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogPRBossPortal, Log, All);

APRBossPortalActor::APRBossPortalActor()
{
	PatternLifeSpan = 0.0f;
}

void APRBossPortalActor::InitializeBossPatternActor(APRBossBaseCharacter* InOwnerBoss, AActor* InPatternTarget)
{
	Super::InitializeBossPatternActor(InOwnerBoss, InPatternTarget);
	SetAndLockPortalTarget(InPatternTarget);
}

void APRBossPortalActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APRBossPortalActor, PortalState);
	DOREPLIFETIME(APRBossPortalActor, LockedTarget);
	DOREPLIFETIME(APRBossPortalActor, bPortalActive);
}

void APRBossPortalActor::RequestPatternActorExpire()
{
	ExpirePortal();
}

void APRBossPortalActor::CancelPatternActor()
{
	ForceExpirePortal();
}

void APRBossPortalActor::SetAndLockPortalTarget(AActor* InTarget)
{
	if (!HasAuthority())
	{
		return;
	}

	LockedTarget = InTarget;

	UE_LOG(LogPRBossPortal, Verbose,
		TEXT("Portal target locked. Portal=%s, Target=%s"),
		*GetNameSafe(this),
		*GetNameSafe(LockedTarget.Get()));
}

void APRBossPortalActor::SetPortalProjectileEffectSpec(const FGameplayEffectSpecHandle& InEffectSpec)
{
	ProjectileEffectSpecHandle = InEffectSpec;
}

void APRBossPortalActor::StartPortalTelegraph()
{
	if (!HasAuthority())
	{
		return;
	}

	if (PortalState == EPRBossPortalState::Telegraphing
		|| PortalState == EPRBossPortalState::Active
		|| PortalState == EPRBossPortalState::Firing
		|| PortalState == EPRBossPortalState::Paused
		|| PortalState == EPRBossPortalState::Expiring)
	{
		return;
	}

	if (!IsValid(LockedTarget) && IsValid(PatternTarget))
	{
		SetAndLockPortalTarget(PatternTarget);
	}

	ClearPortalLifecycleTimers();
	CurrentProjectileFireCount = 0;
	bPortalActive = false;
	PortalState = EPRBossPortalState::Telegraphing;

	UE_LOG(LogPRBossPortal, Verbose,
		TEXT("Portal telegraph started. Portal=%s, ActivationDelay=%.2f"),
		*GetNameSafe(this),
		ActivationDelay);

	MulticastPortalTelegraphStarted();

	if (ActivationDelay <= 0.0f)
	{
		ActivatePortal();
		return;
	}

	GetWorldTimerManager().SetTimer(
		PortalActivationTimerHandle,
		this,
		&APRBossPortalActor::ActivatePortal,
		ActivationDelay,
		false);
}

void APRBossPortalActor::ActivatePortal()
{
	if (!HasAuthority())
	{
		return;
	}

	if (PortalState == EPRBossPortalState::Expiring
		|| PortalState == EPRBossPortalState::Expired
		|| bPortalActive)
	{
		return;
	}

	bPortalActive = true;
	PortalState = EPRBossPortalState::Active;

	UE_LOG(LogPRBossPortal, Verbose,
		TEXT("Portal activated. Portal=%s, ActiveDuration=%.2f"),
		*GetNameSafe(this),
		ActiveDuration);

	MulticastPortalActivated();
	ScheduleNextPortalFire();

	if (ActiveDuration <= 0.0f)
	{
		return;
	}

	GetWorldTimerManager().SetTimer(
		PortalExpireTimerHandle,
		this,
		&APRBossPortalActor::ExpirePortal,
		ActiveDuration,
		false);
}

void APRBossPortalActor::ExpirePortal()
{
	ForceExpirePortal();
}

void APRBossPortalActor::ForceExpirePortal()
{
	if (!HasAuthority())
	{
		return;
	}

	if (PortalState == EPRBossPortalState::Expired
		|| PortalState == EPRBossPortalState::Expiring)
	{
		return;
	}

	ClearPortalLifecycleTimers();

	PortalState = EPRBossPortalState::Expiring;
	bPortalActive = false;

	UE_LOG(LogPRBossPortal, Verbose,
		TEXT("Portal expired. Portal=%s, FiredCount=%d"),
		*GetNameSafe(this),
		CurrentProjectileFireCount);

	MulticastPortalExpired();
	PortalState = EPRBossPortalState::Expired;

	if (bDestroyWhenExpired)
	{
		ExpirePatternActor();
	}
}

void APRBossPortalActor::PausePortal()
{
	if (!HasAuthority())
	{
		return;
	}

	if (PortalState != EPRBossPortalState::Active
		&& PortalState != EPRBossPortalState::Firing)
	{
		return;
	}

	ClearPortalFireTimer();
	PortalState = EPRBossPortalState::Paused;

	UE_LOG(LogPRBossPortal, Verbose,
		TEXT("Portal paused. Portal=%s"),
		*GetNameSafe(this));

	MulticastPortalPaused();
}

void APRBossPortalActor::UnpausePortal()
{
	if (!HasAuthority() || PortalState != EPRBossPortalState::Paused)
	{
		return;
	}

	PortalState = EPRBossPortalState::Active;

	UE_LOG(LogPRBossPortal, Verbose,
		TEXT("Portal unpaused. Portal=%s"),
		*GetNameSafe(this));

	MulticastPortalUnpaused();
	ScheduleNextPortalFire();
}

void APRBossPortalActor::FirePortalProjectile()
{
	if (!HasAuthority())
	{
		return;
	}

	if (!IsValid(LockedTarget))
	{
		UE_LOG(LogPRBossPortal, Verbose,
			TEXT("Portal fire cancelled because target is invalid. Portal=%s"),
			*GetNameSafe(this));
		ForceExpirePortal();
		return;
	}

	if (!CanFirePortalProjectile())
	{
		if (CurrentProjectileFireCount >= MaxProjectilesToFire)
		{
			CompletePortalFireSequence();
		}
		return;
	}

	if (!ProjectileClass->IsChildOf(APRBossProjectileActor::StaticClass()))
	{
		UE_LOG(LogPRBossPortal, Warning,
			TEXT("Portal ProjectileClass should inherit PRBossProjectileActor for default boss damage handling. Portal=%s, ProjectileClass=%s"),
			*GetNameSafe(this),
			*GetNameSafe(ProjectileClass.Get()));
	}

	FTransform ProjectileSpawnTransform;
	if (!BuildProjectileSpawnTransform(ProjectileSpawnTransform))
	{
		UE_LOG(LogPRBossPortal, Warning,
			TEXT("Portal fire failed because spawn transform could not be built. Portal=%s"),
			*GetNameSafe(this));
		CompletePortalFireSequence();
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	const uint32 ProjectileId = NextPortalProjectileId++;
	if (NextPortalProjectileId == 0)
	{
		NextPortalProjectileId = 1;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = IsValid(OwnerBoss) ? static_cast<AActor*>(OwnerBoss) : static_cast<AActor*>(this);
	SpawnParameters.Instigator = IsValid(OwnerBoss) ? OwnerBoss : GetInstigator();
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParameters.CustomPreSpawnInitalization = [ProjectileId](AActor* Actor)
	{
		if (APRProjectileBase* Projectile = Cast<APRProjectileBase>(Actor))
		{
			Projectile->InitializeProjectile(EPRProjectileRole::Auth, ProjectileId);
		}
	};

	PortalState = EPRBossPortalState::Firing;
	MulticastPortalFireStarted();

	APRProjectileBase* SpawnedProjectile = World->SpawnActor<APRProjectileBase>(
		ProjectileClass,
		ProjectileSpawnTransform,
		SpawnParameters);

	if (!IsValid(SpawnedProjectile))
	{
		UE_LOG(LogPRBossPortal, Warning,
			TEXT("Portal projectile spawn failed. Portal=%s, ProjectileClass=%s"),
			*GetNameSafe(this),
			*GetNameSafe(ProjectileClass.Get()));
		PortalState = EPRBossPortalState::Active;
		ScheduleNextPortalFire();
		return;
	}

	SpawnedProjectile->SetProjectileInitialVelocity(CalculateProjectileAimDirection(), ProjectileSpeedOverride);
	if (bUseTrackingProjectile && IsValid(LockedTarget))
	{
		SpawnedProjectile->ConfigureProjectileHoming(LockedTarget->GetRootComponent(), ProjectileHomingAcceleration);
	}

	const FGameplayEffectSpecHandle EffectSpecHandle = ProjectileEffectSpecHandle.IsValid()
		? ProjectileEffectSpecHandle
		: BuildProjectileEffectSpec();
	if (EffectSpecHandle.IsValid())
	{
		SpawnedProjectile->InitGameplayEffectSpec(EffectSpecHandle);
	}

	SpawnedProjectile->PushRepMovement(EPRRepMovementEvent::Spawn);

	++CurrentProjectileFireCount;
	MulticastPortalProjectileSpawned(SpawnedProjectile);

	UE_LOG(LogPRBossPortal, Verbose,
		TEXT("Portal projectile fired. Portal=%s, Projectile=%s, Count=%d/%d"),
		*GetNameSafe(this),
		*GetNameSafe(SpawnedProjectile),
		CurrentProjectileFireCount,
		MaxProjectilesToFire);

	if (CurrentProjectileFireCount >= MaxProjectilesToFire)
	{
		CompletePortalFireSequence();
		return;
	}

	PortalState = EPRBossPortalState::Active;
	ScheduleNextPortalFire();
}

void APRBossPortalActor::ClearPortalFireTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PortalFireTimerHandle);
	}
}

bool APRBossPortalActor::BuildProjectileSpawnTransform(FTransform& OutTransform) const
{
	const FVector SpawnLocation = GetActorTransform().TransformPositionNoScale(ProjectileSpawnLocalOffset);
	const FVector AimDirection = CalculateProjectileAimDirection();
	if (AimDirection.IsNearlyZero())
	{
		return false;
	}

	OutTransform = FTransform((AimDirection.Rotation() + ProjectileRotationOffset), SpawnLocation);
	return true;
}

FVector APRBossPortalActor::CalculateProjectileAimDirection() const
{
	const FVector SpawnLocation = GetActorTransform().TransformPositionNoScale(ProjectileSpawnLocalOffset);
	const AActor* TargetActor = IsValid(LockedTarget) ? LockedTarget.Get() : PatternTarget.Get();
	if (!IsValid(TargetActor))
	{
		return GetActorForwardVector();
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

bool APRBossPortalActor::CanFirePortalProjectile() const
{
	if (!HasAuthority()
		|| !bPortalActive
		|| !IsValid(ProjectileClass)
		|| !IsValid(LockedTarget)
		|| MaxProjectilesToFire <= 0
		|| CurrentProjectileFireCount >= MaxProjectilesToFire)
	{
		return false;
	}

	if (PortalState == EPRBossPortalState::Paused && !bCanFireWhilePaused)
	{
		return false;
	}

	return PortalState == EPRBossPortalState::Active
		|| PortalState == EPRBossPortalState::Firing
		|| (PortalState == EPRBossPortalState::Paused && bCanFireWhilePaused);
}

void APRBossPortalActor::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority() && bAutoStartPortal)
	{
		StartPortalTelegraph();
	}
}

void APRBossPortalActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearPortalLifecycleTimers();
	Super::EndPlay(EndPlayReason);
}

void APRBossPortalActor::MulticastPortalTelegraphStarted_Implementation()
{
	BP_OnPortalTelegraphStarted();
}

void APRBossPortalActor::MulticastPortalActivated_Implementation()
{
	BP_OnPortalActivated();
}

void APRBossPortalActor::MulticastPortalExpired_Implementation()
{
	BP_OnPortalExpired();
}

void APRBossPortalActor::MulticastPortalFireStarted_Implementation()
{
	BP_OnPortalFireStarted();
}

void APRBossPortalActor::MulticastPortalProjectileSpawned_Implementation(APRProjectileBase* SpawnedProjectile)
{
	BP_OnPortalProjectileSpawned(SpawnedProjectile);
}

void APRBossPortalActor::MulticastPortalPaused_Implementation()
{
	BP_OnPortalPaused();
}

void APRBossPortalActor::MulticastPortalUnpaused_Implementation()
{
	BP_OnPortalUnpaused();
}

void APRBossPortalActor::MulticastPortalFireSequenceCompleted_Implementation()
{
	BP_OnPortalFireSequenceCompleted();
}

void APRBossPortalActor::ScheduleNextPortalFire()
{
	if (!HasAuthority() || !CanFirePortalProjectile())
	{
		return;
	}

	ClearPortalFireTimer();

	const float FireDelay = CurrentProjectileFireCount == 0 ? InitialFireDelay : FireInterval;
	if (FireDelay <= 0.0f)
	{
		FirePortalProjectile();
		return;
	}

	GetWorldTimerManager().SetTimer(
		PortalFireTimerHandle,
		this,
		&APRBossPortalActor::FirePortalProjectile,
		FireDelay,
		false);
}

void APRBossPortalActor::CompletePortalFireSequence()
{
	if (!HasAuthority())
	{
		return;
	}

	ClearPortalFireTimer();
	MulticastPortalFireSequenceCompleted();

	UE_LOG(LogPRBossPortal, Verbose,
		TEXT("Portal fire sequence completed. Portal=%s, FiredCount=%d"),
		*GetNameSafe(this),
		CurrentProjectileFireCount);

	if (bDestroyAfterFireComplete)
	{
		ForceExpirePortal();
		return;
	}

	if (PortalState != EPRBossPortalState::Expired && PortalState != EPRBossPortalState::Expiring)
	{
		PortalState = EPRBossPortalState::Active;
	}
}

void APRBossPortalActor::ClearPortalLifecycleTimers()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PortalActivationTimerHandle);
		World->GetTimerManager().ClearTimer(PortalExpireTimerHandle);
		World->GetTimerManager().ClearTimer(PortalFireTimerHandle);
	}
}

FGameplayEffectSpecHandle APRBossPortalActor::BuildProjectileEffectSpec() const
{
	if (!IsValid(OwnerBoss))
	{
		UE_LOG(LogPRBossPortal, Warning,
			TEXT("Portal projectile damage spec build failed because OwnerBoss is invalid. Portal=%s"),
			*GetNameSafe(this));
		return FGameplayEffectSpecHandle();
	}

	UPRAbilitySystemComponent* SourceASC = OwnerBoss->GetEnemyAbilitySystemComponent();
	if (!IsValid(SourceASC))
	{
		UE_LOG(LogPRBossPortal, Warning,
			TEXT("Portal projectile damage spec build failed because SourceASC is invalid. Portal=%s, OwnerBoss=%s"),
			*GetNameSafe(this),
			*GetNameSafe(OwnerBoss.Get()));
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
		UE_LOG(LogPRBossPortal, Warning,
			TEXT("Portal projectile damage spec build failed because DamageEffectClass is invalid. Portal=%s"),
			*GetNameSafe(this));
		return FGameplayEffectSpecHandle();
	}

	FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
	EffectContext.AddSourceObject(const_cast<APRBossPortalActor*>(this));

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(
		ResolvedDamageEffectClass,
		1.0f,
		EffectContext);

	if (SpecHandle.IsValid())
	{
		SpecHandle.Data->SetSetByCallerMagnitude(
			PRCombatGameplayTags::SetByCaller_AttackMultiplier,
			ProjectileAttackMultiplier);
	}

	return SpecHandle;
}

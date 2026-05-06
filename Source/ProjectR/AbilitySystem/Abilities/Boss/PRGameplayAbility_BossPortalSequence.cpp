// Copyright ProjectR. All Rights Reserved.

#include "PRGameplayAbility_BossPortalSequence.h"

#include "Engine/World.h"
#include "TimerManager.h"
#include "ProjectR/AI/Boss/PRBossPortalActor.h"

DEFINE_LOG_CATEGORY_STATIC(LogPRBossPortalSequence, Log, All);

UPRGameplayAbility_BossPortalSequence::UPRGameplayAbility_BossPortalSequence()
{
}

void UPRGameplayAbility_BossPortalSequence::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CanRunBossPattern() || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	SpawnedPatternActors.Reset();
	SpawnedPortalRefs.Reset();

	AActor* PatternTarget = GetBossPatternTarget();
	for (const FPRBossPatternActorSpawnConfig& SpawnConfig : PatternActorSpawnConfigs)
	{
		APRBossPatternActor* SpawnedActor = SpawnPatternActor(SpawnConfig);
		if (!IsValid(SpawnedActor))
		{
			continue;
		}

		SpawnedPatternActors.Add(SpawnedActor);

		APRBossPortalActor* SpawnedPortal = Cast<APRBossPortalActor>(SpawnedActor);
		if (!IsValid(SpawnedPortal))
		{
			UE_LOG(LogPRBossPortalSequence, Warning,
				TEXT("BossPortalSequence spawned non-portal actor. Ability=%s, Actor=%s"),
				*GetNameSafe(this),
				*GetNameSafe(SpawnedActor));
			continue;
		}

		SpawnedPortal->SetAndLockPortalTarget(PatternTarget);
		SpawnedPortalRefs.Add(SpawnedPortal);

		if (bStartPortalsAfterSpawn)
		{
			SpawnedPortal->StartPortalTelegraph();
		}
	}

	TArray<APRBossPatternActor*> SpawnedActorsForEvent;
	SpawnedActorsForEvent.Reserve(SpawnedPatternActors.Num());
	for (APRBossPatternActor* SpawnedActor : SpawnedPatternActors)
	{
		if (IsValid(SpawnedActor))
		{
			SpawnedActorsForEvent.Add(SpawnedActor);
		}
	}

	BP_OnPatternActorsSpawned(SpawnedActorsForEvent);

	if (PortalSequenceDuration <= 0.0f)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			PortalSequenceTimerHandle,
			this,
			&UPRGameplayAbility_BossPortalSequence::FinishPortalSequence,
			PortalSequenceDuration,
			false);
	}
}

void UPRGameplayAbility_BossPortalSequence::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PortalSequenceTimerHandle);
	}

	if (bExpireSpawnedPortalsOnEnd)
	{
		ExpireSpawnedPortals();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UPRGameplayAbility_BossPortalSequence::FinishPortalSequence()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UPRGameplayAbility_BossPortalSequence::ExpireSpawnedPortals()
{
	if (!HasAuthority(&CurrentActivationInfo))
	{
		return;
	}

	for (APRBossPatternActor* SpawnedActor : SpawnedPatternActors)
	{
		if (IsValid(SpawnedActor))
		{
			SpawnedActor->CancelPatternActor();
		}
	}

	SpawnedPortalRefs.Reset();
	SpawnedPatternActors.Reset();
}

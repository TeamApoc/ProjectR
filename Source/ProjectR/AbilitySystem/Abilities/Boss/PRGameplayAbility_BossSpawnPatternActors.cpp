// Copyright ProjectR. All Rights Reserved.

#include "PRGameplayAbility_BossSpawnPatternActors.h"

#include "Engine/World.h"
#include "ProjectR/AI/Boss/PRBossPatternActor.h"
#include "ProjectR/Character/Enemy/PRBossBaseCharacter.h"

UPRGameplayAbility_BossSpawnPatternActors::UPRGameplayAbility_BossSpawnPatternActors()
{
}

void UPRGameplayAbility_BossSpawnPatternActors::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
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

	for (const FPRBossPatternActorSpawnConfig& SpawnConfig : PatternActorSpawnConfigs)
	{
		if (APRBossPatternActor* SpawnedActor = SpawnPatternActor(SpawnConfig))
		{
			SpawnedPatternActors.Add(SpawnedActor);
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
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

bool UPRGameplayAbility_BossSpawnPatternActors::BuildPatternActorSpawnTransform(
	const FPRBossPatternActorSpawnConfig& SpawnConfig,
	FTransform& OutSpawnTransform) const
{
	const APRBossBaseCharacter* BossCharacter = GetBossAvatarCharacter();
	AActor* PatternTarget = GetBossPatternTarget();

	if (!IsValid(BossCharacter))
	{
		return false;
	}

	const AActor* OriginActor = BossCharacter;
	if (SpawnConfig.SpawnOrigin == EPRBossPatternSpawnOrigin::Target && IsValid(PatternTarget))
	{
		OriginActor = PatternTarget;
	}

	const FTransform OriginTransform = OriginActor->GetActorTransform();
	const FVector SpawnLocation = OriginTransform.TransformPositionNoScale(SpawnConfig.LocalOffset);

	FRotator SpawnRotation = OriginTransform.GetRotation().Rotator();
	if (SpawnConfig.bFaceTarget && IsValid(PatternTarget))
	{
		const FVector DirectionToTarget = PatternTarget->GetActorLocation() - SpawnLocation;
		if (!DirectionToTarget.IsNearlyZero())
		{
			SpawnRotation = FRotationMatrix::MakeFromX(DirectionToTarget).Rotator();
			if (SpawnConfig.bUseYawOnlyFacing)
			{
				SpawnRotation.Pitch = 0.0f;
				SpawnRotation.Roll = 0.0f;
			}
		}
	}

	OutSpawnTransform = FTransform(SpawnRotation + SpawnConfig.RotationOffset, SpawnLocation);
	return true;
}

APRBossPatternActor* UPRGameplayAbility_BossSpawnPatternActors::SpawnPatternActor(
	const FPRBossPatternActorSpawnConfig& SpawnConfig)
{
	APRBossBaseCharacter* BossCharacter = GetBossAvatarCharacter();
	if (!IsValid(BossCharacter) || !BossCharacter->HasAuthority() || !SpawnConfig.PatternActorClass)
	{
		return nullptr;
	}

	UWorld* World = BossCharacter->GetWorld();
	if (!IsValid(World))
	{
		return nullptr;
	}

	FTransform SpawnTransform;
	if (!BuildPatternActorSpawnTransform(SpawnConfig, SpawnTransform))
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = BossCharacter;
	SpawnParameters.Instigator = BossCharacter;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	APRBossPatternActor* SpawnedActor = World->SpawnActor<APRBossPatternActor>(
		SpawnConfig.PatternActorClass,
		SpawnTransform,
		SpawnParameters);

	if (IsValid(SpawnedActor))
	{
		SpawnedActor->InitializeBossPatternActor(BossCharacter, GetBossPatternTarget());
	}

	return SpawnedActor;
}

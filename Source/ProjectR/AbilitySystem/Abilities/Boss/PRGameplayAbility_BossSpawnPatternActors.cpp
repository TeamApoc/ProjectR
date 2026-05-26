// Copyright ProjectR. All Rights Reserved.

#include "PRGameplayAbility_BossSpawnPatternActors.h"

#include "Engine/World.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "ProjectR/AI/PREnemyEQSSelectionUtils.h"
#include "ProjectR/AI/Boss/PRBossPatternActor.h"
#include "ProjectR/Character/Enemy/PRBossBaseCharacter.h"

DEFINE_LOG_CATEGORY_STATIC(LogPRBossSpawnPatternActors, Log, All);

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
	if (SpawnConfig.SpawnLocationMode != EPRBossPatternSpawnLocationMode::EnvQuery)
	{
		return BuildOriginOffsetSpawnTransform(SpawnConfig, OutSpawnTransform);
	}

	FVector SpawnLocation = FVector::ZeroVector;
	if (!RunSpawnLocationQuery(SpawnConfig, SpawnLocation))
	{
		if (SpawnConfig.bFallbackToOriginOffsetWhenQueryFails)
		{
			return BuildOriginOffsetSpawnTransform(SpawnConfig, OutSpawnTransform);
		}

		return false;
	}

	const APRBossBaseCharacter* BossCharacter = GetBossAvatarCharacter();
	AActor* PatternTarget = GetBossPatternTarget();
	if (!IsValid(BossCharacter))
	{
		return false;
	}

	FRotator SpawnRotation = BossCharacter->GetActorRotation();
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

bool UPRGameplayAbility_BossSpawnPatternActors::BuildOriginOffsetSpawnTransform(
	const FPRBossPatternActorSpawnConfig& SpawnConfig,
	FTransform& OutSpawnTransform) const
{
	const APRBossBaseCharacter* BossCharacter = GetBossAvatarCharacter();
	AActor* PatternTarget = GetBossPatternTarget();

	if (!IsValid(BossCharacter))
	{
		return false;
	}

	const AActor* OriginActor = ResolveSpawnOriginActor(SpawnConfig);
	if (!IsValid(OriginActor))
	{
		return false;
	}

	const FTransform OriginTransform = OriginActor->GetActorTransform();
	const FVector SpawnLocation = SpawnConfig.bUseWorldSpaceOffset
		? OriginActor->GetActorLocation() + SpawnConfig.LocalOffset
		: OriginTransform.TransformPositionNoScale(SpawnConfig.LocalOffset);

	FRotator SpawnRotation = SpawnConfig.bUseWorldSpaceRotation
		? FRotator::ZeroRotator
		: OriginTransform.GetRotation().Rotator();
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

bool UPRGameplayAbility_BossSpawnPatternActors::RunSpawnLocationQuery(
	const FPRBossPatternActorSpawnConfig& SpawnConfig,
	FVector& OutSpawnLocation) const
{
	APRBossBaseCharacter* BossCharacter = GetBossAvatarCharacter();
	if (!IsValid(BossCharacter))
	{
		return false;
	}

	if (!PREnemyEQSSelectionUtils::RunLocationQuery(
			BossCharacter,
			SpawnConfig.SpawnQueryTemplate.Get(),
			SpawnConfig.FloatParams,
			SpawnConfig.SpawnQueryRunMode,
			SpawnConfig.CandidateSelectionMode,
			SpawnConfig.TopCandidateCount,
			SpawnConfig.TopScoreCandidateRatio,
			OutSpawnLocation))
	{
		UE_LOG(LogPRBossSpawnPatternActors, Verbose,
			TEXT("Boss pattern spawn EQS failed. Ability=%s, Query=%s"),
			*GetNameSafe(this),
			*GetNameSafe(SpawnConfig.SpawnQueryTemplate.Get()));
		return false;
	}

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
		ApplyPostSpawnAttachment(SpawnedActor, SpawnConfig);
	}

	return SpawnedActor;
}

AActor* UPRGameplayAbility_BossSpawnPatternActors::ResolveSpawnOriginActor(
	const FPRBossPatternActorSpawnConfig& SpawnConfig) const
{
	APRBossBaseCharacter* BossCharacter = GetBossAvatarCharacter();
	if (!IsValid(BossCharacter))
	{
		return nullptr;
	}

	AActor* PatternTarget = GetBossPatternTarget();
	if (SpawnConfig.SpawnOrigin == EPRBossPatternSpawnOrigin::Target && IsValid(PatternTarget))
	{
		return PatternTarget;
	}

	return BossCharacter;
}

void UPRGameplayAbility_BossSpawnPatternActors::ApplyPostSpawnAttachment(
	APRBossPatternActor* SpawnedActor,
	const FPRBossPatternActorSpawnConfig& SpawnConfig) const
{
	if (!IsValid(SpawnedActor) || !SpawnConfig.bAttachToOriginAfterSpawn)
	{
		return;
	}

	AActor* OriginActor = ResolveSpawnOriginActor(SpawnConfig);
	if (!IsValid(OriginActor))
	{
		return;
	}

	SpawnedActor->AttachToActor(OriginActor, FAttachmentTransformRules::KeepWorldTransform);
}

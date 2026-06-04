// Copyright ProjectR. All Rights Reserved.

#include "PRGameplayAbility_FaerinRainPortalSequence.h"

#include "ProjectR/AbilitySystem/Abilities/Boss/PRBossAbilityTagUtils.h"
#include "ProjectR/AI/Boss/PRBossPortalActor.h"
#include "ProjectR/PRGameplayTags.h"

UPRGameplayAbility_FaerinRainPortalSequence::UPRGameplayAbility_FaerinRainPortalSequence()
{
	SetAssetTags(PRBossAbility::MakePatternAssetTags(PRGameplayTags::Ability_Boss_Faerin_RainPortalSequence));
	ActivationBlockedTags.AddTag(PRGameplayTags::Cooldown_Boss_Faerin_RainPortal);

	PortalPatternType = EPRBossPortalPatternType::Torrent;
	SpawnTimingMode = EPRBossPortalSpawnTimingMode::ImmediateOnActivate;
	bStartPortalsAfterSpawn = true;
	PortalStartInterval = 0.0f;
	bExpireSpawnedPortalsOnEnd = false;
	PortalSequenceDuration = 0.0f;
	DownwardPortalRotation = FVector::DownVector.Rotation();
}

void UPRGameplayAbility_FaerinRainPortalSequence::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!IsValid(RainPortalActorClass) || PortalCount <= 0)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	RebuildRainPortalSpawnConfigs();
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UPRGameplayAbility_FaerinRainPortalSequence::StartSpawnedPortals()
{
	if (!bStartPortalsAfterSpawn)
	{
		return;
	}

	const int32 ResolvedGroupSize = FMath::Max(PortalStartGroupSize, 1);
	int32 PortalIndex = 0;
	for (APRBossPortalActor* SpawnedPortal : SpawnedPortalRefs)
	{
		if (!IsValid(SpawnedPortal))
		{
			continue;
		}

		const int32 GroupIndex = PortalIndex / ResolvedGroupSize;
		const float StartDelay = FMath::Max(PortalStartGroupInterval, 0.0f) * static_cast<float>(GroupIndex);
		SpawnedPortal->StartPortalTelegraphAfterDelay(StartDelay);
		++PortalIndex;
	}
}

void UPRGameplayAbility_FaerinRainPortalSequence::RebuildRainPortalSpawnConfigs()
{
	PatternActorSpawnConfigs.Reset();

	const int32 ResolvedPortalCount = FMath::Max(PortalCount, 1);
	for (int32 PortalIndex = 0; PortalIndex < ResolvedPortalCount; ++PortalIndex)
	{
		FPRBossPatternActorSpawnConfig& SpawnConfig = PatternActorSpawnConfigs.AddDefaulted_GetRef();
		SpawnConfig.PatternActorClass = RainPortalActorClass;
		SpawnConfig.SpawnLocationMode = EPRBossPatternSpawnLocationMode::OriginOffset;
		SpawnConfig.SpawnOrigin = RainPortalSpawnOrigin;
		SpawnConfig.LocalOffset = BuildRainPortalOffset(PortalIndex, ResolvedPortalCount);
		SpawnConfig.bUseWorldSpaceOffset = true;
		SpawnConfig.RotationOffset = DownwardPortalRotation;
		SpawnConfig.bUseWorldSpaceRotation = true;
		SpawnConfig.bFaceTarget = false;
	}
}

FVector UPRGameplayAbility_FaerinRainPortalSequence::BuildRainPortalOffset(
	const int32 PortalIndex,
	const int32 ResolvedPortalCount) const
{
	const float OuterRadius = FMath::Max(SpawnAreaRadius, 0.0f);

	if (ResolvedPortalCount <= 1)
	{
		return FVector(0.0f, 0.0f, SpawnHeight);
	}

	constexpr float GoldenAngleRadians = 2.39996323f;
	const float NormalizedIndex = static_cast<float>(PortalIndex) / static_cast<float>(ResolvedPortalCount - 1);
	const float Radius = FMath::Sqrt(NormalizedIndex) * OuterRadius;
	const float Angle = static_cast<float>(PortalIndex) * GoldenAngleRadians;
	return FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, SpawnHeight);
}

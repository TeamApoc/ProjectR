// Copyright ProjectR. All Rights Reserved.

#include "PRGameplayAbility_FaerinPortalPairSequence.h"

#include "ProjectR/AbilitySystem/Abilities/Boss/PRBossAbilityTagUtils.h"
#include "ProjectR/PRGameplayTags.h"

UPRGameplayAbility_FaerinPortalPairSequence::UPRGameplayAbility_FaerinPortalPairSequence()
{
	SetAssetTags(PRBossAbility::MakePatternAssetTags(PRGameplayTags::Ability_Boss_Faerin_PortalPairSequence));

	PortalPatternType = EPRBossPortalPatternType::Pair;
	SpawnTimingMode = EPRBossPortalSpawnTimingMode::OnCharacterEvent;
	SpawnCharacterEventName = TEXT("SummonPortal_Missile");
	bEndAbilityAfterEventSpawn = false;
	bEndAbilityOnSummonMontageCompleted = true;
	bStartPortalsAfterSpawn = true;
	PortalStartInterval = 0.0f;
	bExpireSpawnedPortalsOnEnd = false;
	PortalSequenceDuration = 0.0f;

	PatternActorSpawnConfigs.Reset();

	FPRBossPatternActorSpawnConfig& LeftPortalConfig = PatternActorSpawnConfigs.AddDefaulted_GetRef();
	LeftPortalConfig.SpawnLocationMode = EPRBossPatternSpawnLocationMode::OriginOffset;
	LeftPortalConfig.SpawnOrigin = EPRBossPatternSpawnOrigin::Target;
	LeftPortalConfig.LocalOffset = FVector(-350.0f, -260.0f, 450.0f);
	LeftPortalConfig.bFaceTarget = true;
	LeftPortalConfig.bUseYawOnlyFacing = true;

	FPRBossPatternActorSpawnConfig& RightPortalConfig = PatternActorSpawnConfigs.AddDefaulted_GetRef();
	RightPortalConfig.SpawnLocationMode = EPRBossPatternSpawnLocationMode::OriginOffset;
	RightPortalConfig.SpawnOrigin = EPRBossPatternSpawnOrigin::Target;
	RightPortalConfig.LocalOffset = FVector(-350.0f, 260.0f, 450.0f);
	RightPortalConfig.bFaceTarget = true;
	RightPortalConfig.bUseYawOnlyFacing = true;
}

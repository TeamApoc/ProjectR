// Copyright ProjectR. All Rights Reserved.

#include "PRGameplayAbility_FaerinPortalAttachedSequence.h"

#include "ProjectR/AbilitySystem/Abilities/Boss/PRBossAbilityTagUtils.h"
#include "ProjectR/PRGameplayTags.h"

UPRGameplayAbility_FaerinPortalAttachedSequence::UPRGameplayAbility_FaerinPortalAttachedSequence()
{
	SetAssetTags(PRBossAbility::MakePatternAssetTags(PRGameplayTags::Ability_Boss_Faerin_PortalAttachedSequence));
	ActivationBlockedTags.AddTag(PRGameplayTags::Cooldown_Boss_Faerin_PortalAttached);

	PortalPatternType = EPRBossPortalPatternType::Attached;
	SpawnTimingMode = EPRBossPortalSpawnTimingMode::OnCharacterEvent;
	SpawnCharacterEventName = TEXT("SummonPortal_Attached");
	bEndAbilityAfterEventSpawn = false;
	bEndAbilityOnSummonMontageCompleted = true;
	bStartPortalsAfterSpawn = true;
	PortalStartInterval = 0.0f;
	bExpireSpawnedPortalsOnEnd = false;
	PortalSequenceDuration = 0.0f;

	PatternActorSpawnConfigs.Reset();

	FPRBossPatternActorSpawnConfig& LeftPortalConfig = PatternActorSpawnConfigs.AddDefaulted_GetRef();
	LeftPortalConfig.SpawnLocationMode = EPRBossPatternSpawnLocationMode::OriginOffset;
	LeftPortalConfig.SpawnOrigin = EPRBossPatternSpawnOrigin::Boss;
	LeftPortalConfig.LocalOffset = FVector(80.0f, -260.0f, 320.0f);
	LeftPortalConfig.bFaceTarget = true;
	LeftPortalConfig.bUseYawOnlyFacing = true;
	LeftPortalConfig.bAttachToOriginAfterSpawn = true;

	FPRBossPatternActorSpawnConfig& RightPortalConfig = PatternActorSpawnConfigs.AddDefaulted_GetRef();
	RightPortalConfig.SpawnLocationMode = EPRBossPatternSpawnLocationMode::OriginOffset;
	RightPortalConfig.SpawnOrigin = EPRBossPatternSpawnOrigin::Boss;
	RightPortalConfig.LocalOffset = FVector(80.0f, 260.0f, 320.0f);
	RightPortalConfig.bFaceTarget = true;
	RightPortalConfig.bUseYawOnlyFacing = true;
	RightPortalConfig.bAttachToOriginAfterSpawn = true;
}


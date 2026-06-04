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
	LeftPortalConfig.OriginComponentName = TEXT("PortalAttach_Left");
	LeftPortalConfig.LocalOffset = FVector::ZeroVector;
	LeftPortalConfig.bFaceTarget = false;
	LeftPortalConfig.bAttachToOriginAfterSpawn = true;

	FPRBossPatternActorSpawnConfig& RightPortalConfig = PatternActorSpawnConfigs.AddDefaulted_GetRef();
	RightPortalConfig.SpawnLocationMode = EPRBossPatternSpawnLocationMode::OriginOffset;
	RightPortalConfig.SpawnOrigin = EPRBossPatternSpawnOrigin::Boss;
	RightPortalConfig.OriginComponentName = TEXT("PortalAttach_Right");
	RightPortalConfig.LocalOffset = FVector::ZeroVector;
	RightPortalConfig.bFaceTarget = false;
	RightPortalConfig.bAttachToOriginAfterSpawn = true;
}


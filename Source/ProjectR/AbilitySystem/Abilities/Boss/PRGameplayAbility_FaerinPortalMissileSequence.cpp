// Copyright ProjectR. All Rights Reserved.

#include "PRGameplayAbility_FaerinPortalMissileSequence.h"

#include "ProjectR/AbilitySystem/Abilities/Boss/PRBossAbilityTagUtils.h"
#include "ProjectR/PRGameplayTags.h"

UPRGameplayAbility_FaerinPortalMissileSequence::UPRGameplayAbility_FaerinPortalMissileSequence()
{
	SetAssetTags(PRBossAbility::MakePatternAssetTags(PRGameplayTags::Ability_Boss_Faerin_PortalMissileSequence));

	PortalPatternType = EPRBossPortalPatternType::Missile;
	SpawnTimingMode = EPRBossPortalSpawnTimingMode::OnCharacterEvent;
	SpawnCharacterEventName = TEXT("SummonPortal_Missile");
	bEndAbilityAfterEventSpawn = false;
	bEndAbilityOnSummonMontageCompleted = true;
	bStartPortalsAfterSpawn = true;
	PortalStartInterval = 0.0f;
	bExpireSpawnedPortalsOnEnd = false;
	PortalSequenceDuration = 0.0f;

	PatternActorSpawnConfigs.Reset();
	FPRBossPatternActorSpawnConfig& SpawnConfig = PatternActorSpawnConfigs.AddDefaulted_GetRef();
	SpawnConfig.SpawnLocationMode = EPRBossPatternSpawnLocationMode::OriginOffset;
	SpawnConfig.SpawnOrigin = EPRBossPatternSpawnOrigin::Target;
	SpawnConfig.LocalOffset = FVector(-350.0f, 0.0f, 450.0f);
	SpawnConfig.bFaceTarget = true;
	SpawnConfig.bUseYawOnlyFacing = true;
}

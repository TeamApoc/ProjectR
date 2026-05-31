// Copyright ProjectR. All Rights Reserved.

#include "PRGameplayAbility_FaerinCrashDropSequence.h"

#include "ProjectR/AbilitySystem/Abilities/Boss/PRBossAbilityTagUtils.h"
#include "ProjectR/PRGameplayTags.h"

UPRGameplayAbility_FaerinCrashDropSequence::UPRGameplayAbility_FaerinCrashDropSequence()
{
	SetAssetTags(PRBossAbility::MakePatternAssetTags(PRGameplayTags::Ability_Boss_Faerin_CrashDropSequence));
	CrashDamageMode = EPRFaerinCrashDamageMode::GlobalPlayers;
}

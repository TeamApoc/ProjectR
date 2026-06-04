// Copyright ProjectR. All Rights Reserved.

#include "PRGameplayAbility_FaerinTeleportDownSequence.h"

#include "ProjectR/AbilitySystem/Abilities/Boss/PRBossAbilityTagUtils.h"
#include "ProjectR/PRGameplayTags.h"

UPRGameplayAbility_FaerinTeleportDownSequence::UPRGameplayAbility_FaerinTeleportDownSequence()
{
	SetAssetTags(PRBossAbility::MakePatternAssetTags(PRGameplayTags::Ability_Boss_Faerin_TeleportDownSequence));
}

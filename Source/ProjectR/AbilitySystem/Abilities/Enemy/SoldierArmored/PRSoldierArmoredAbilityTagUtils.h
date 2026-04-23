// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "ProjectR/PRGameplayTags.h"

namespace PRSoldierArmoredAbility
{
	inline FGameplayTagContainer MakePatternAssetTags(const FGameplayTag& PatternTag)
	{
		// 상태 Ability 일괄 취소용 Soldier_Armored 공통/개별 패턴 태그 묶음
		FGameplayTagContainer AssetTags;
		AssetTags.AddTag(PRGameplayTags::Ability_Enemy_SoldierArmored_Pattern);
		AssetTags.AddTag(PatternTag);
		return AssetTags;
	}
}

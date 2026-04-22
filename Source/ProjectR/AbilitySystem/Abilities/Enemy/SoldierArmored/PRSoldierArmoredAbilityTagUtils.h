// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "ProjectR/PRGameplayTags.h"

namespace PRSoldierArmoredAbility
{
	inline FGameplayTagContainer MakePatternAssetTags(const FGameplayTag& PatternTag)
	{
		// Soldier_Armored 공통 패턴 태그와 개별 공격 태그를 함께 넣어 상태 Ability에서 일괄 취소할 수 있게 한다.
		FGameplayTagContainer AssetTags;
		AssetTags.AddTag(PRGameplayTags::Ability_Enemy_SoldierArmored_Pattern);
		AssetTags.AddTag(PatternTag);
		return AssetTags;
	}
}

// Copyright ProjectR. All Rights Reserved.

#include "PRSoldierArmoredPatternDataAsset.h"

#include "ProjectR/PRGameplayTags.h"

UPRSoldierArmoredPatternDataAsset::UPRSoldierArmoredPatternDataAsset()
{
	// 기본 패턴은 근접 3종과 중거리 돌진 1종이다.
	// 에디터 DataAsset에서 같은 배열을 조정하면 몬스터 튜닝을 코드 수정 없이 할 수 있다.
	PatternRules.Reset();

	FPRPatternRule HammerSwing01;
	HammerSwing01.AbilityTag = PRGameplayTags::Ability_Enemy_SoldierArmored_HammerSwing01;
	HammerSwing01.PatternCategory = EPRPatternCategory::Melee;
	HammerSwing01.MinRange = 0.0f;
	HammerSwing01.MaxRange = 220.0f;
	HammerSwing01.bRequiresLOS = true;
	HammerSwing01.RequiredComboIndex = 0;
	HammerSwing01.NextComboIndex = 1;
	HammerSwing01.SelectionWeight = 1.0f;
	PatternRules.Add(HammerSwing01);

	FPRPatternRule HammerSwing02;
	HammerSwing02.AbilityTag = PRGameplayTags::Ability_Enemy_SoldierArmored_HammerSwing02;
	HammerSwing02.PatternCategory = EPRPatternCategory::Melee;
	HammerSwing02.MinRange = 0.0f;
	HammerSwing02.MaxRange = 240.0f;
	HammerSwing02.bRequiresLOS = true;
	HammerSwing02.RequiredComboIndex = 1;
	HammerSwing02.NextComboIndex = 2;
	HammerSwing02.SelectionWeight = 0.85f;
	PatternRules.Add(HammerSwing02);

	FPRPatternRule HammerOverhead;
	HammerOverhead.AbilityTag = PRGameplayTags::Ability_Enemy_SoldierArmored_HammerOverhead;
	HammerOverhead.PatternCategory = EPRPatternCategory::Melee;
	HammerOverhead.MinRange = 0.0f;
	HammerOverhead.MaxRange = 260.0f;
	HammerOverhead.bRequiresLOS = true;
	HammerOverhead.RequiredComboIndex = 2;
	HammerOverhead.NextComboIndex = 0;
	HammerOverhead.SelectionWeight = 0.45f;
	PatternRules.Add(HammerOverhead);

	FPRPatternRule ChargeThrust;
	// 돌진은 너무 가까운 거리에서는 쓰지 않도록 MinRange를 둔다.
	ChargeThrust.AbilityTag = PRGameplayTags::Ability_Enemy_SoldierArmored_ChargeThrust;
	ChargeThrust.PatternCategory = EPRPatternCategory::Sprint;
	ChargeThrust.MinRange = 350.0f;
	ChargeThrust.MaxRange = 850.0f;
	ChargeThrust.bRequiresLOS = true;
	ChargeThrust.bRequiresChargePathClear = true;
	ChargeThrust.NextComboIndex = 0;
	ChargeThrust.SelectionWeight = 0.65f;
	PatternRules.Add(ChargeThrust);
}

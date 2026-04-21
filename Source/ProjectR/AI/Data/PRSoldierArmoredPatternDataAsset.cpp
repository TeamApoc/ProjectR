// Copyright ProjectR. All Rights Reserved.

#include "PRSoldierArmoredPatternDataAsset.h"

#include "ProjectR/PRGameplayTags.h"

UPRSoldierArmoredPatternDataAsset::UPRSoldierArmoredPatternDataAsset()
{
	PatternRules.Reset();

	FPRPatternRule HammerSwing01;
	HammerSwing01.AbilityTag = PRGameplayTags::Ability_Enemy_SoldierArmored_HammerSwing01;
	HammerSwing01.MinRange = 0.0f;
	HammerSwing01.MaxRange = 220.0f;
	HammerSwing01.bRequiresLOS = true;
	HammerSwing01.SelectionWeight = 1.0f;
	PatternRules.Add(HammerSwing01);

	FPRPatternRule HammerSwing02;
	HammerSwing02.AbilityTag = PRGameplayTags::Ability_Enemy_SoldierArmored_HammerSwing02;
	HammerSwing02.MinRange = 0.0f;
	HammerSwing02.MaxRange = 240.0f;
	HammerSwing02.bRequiresLOS = true;
	HammerSwing02.SelectionWeight = 0.85f;
	PatternRules.Add(HammerSwing02);

	FPRPatternRule HammerOverhead;
	HammerOverhead.AbilityTag = PRGameplayTags::Ability_Enemy_SoldierArmored_HammerOverhead;
	HammerOverhead.MinRange = 0.0f;
	HammerOverhead.MaxRange = 260.0f;
	HammerOverhead.bRequiresLOS = true;
	HammerOverhead.SelectionWeight = 0.45f;
	PatternRules.Add(HammerOverhead);

	FPRPatternRule ChargeThrust;
	ChargeThrust.AbilityTag = PRGameplayTags::Ability_Enemy_SoldierArmored_ChargeThrust;
	ChargeThrust.MinRange = 350.0f;
	ChargeThrust.MaxRange = 850.0f;
	ChargeThrust.bRequiresLOS = true;
	ChargeThrust.SelectionWeight = 0.65f;
	PatternRules.Add(ChargeThrust);
}

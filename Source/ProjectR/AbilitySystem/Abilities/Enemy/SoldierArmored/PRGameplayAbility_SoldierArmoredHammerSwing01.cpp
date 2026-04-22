// Copyright ProjectR. All Rights Reserved.

#include "PRGameplayAbility_SoldierArmoredHammerSwing01.h"

#include "ProjectR/AbilitySystem/Abilities/Enemy/SoldierArmored/PRSoldierArmoredAbilityTagUtils.h"
#include "ProjectR/PRGameplayTags.h"

UPRGameplayAbility_SoldierArmoredHammerSwing01::UPRGameplayAbility_SoldierArmoredHammerSwing01()
{
	SetAssetTags(PRSoldierArmoredAbility::MakePatternAssetTags(PRGameplayTags::Ability_Enemy_SoldierArmored_HammerSwing01));
	AbilityTag = PRGameplayTags::Ability_Enemy_SoldierArmored_HammerSwing01;

	// BP 자식이 아직 없을 때도 기본 전투 테스트가 가능하도록 둔 임시 기본값이다.
	Damage = 18.0f;
	GroggyDamage = 16.0f;
	WindupTime = 0.25f;
	RecoveryTime = 0.35f;
	AttackRange = 220.0f;
	AttackRadius = 80.0f;
}

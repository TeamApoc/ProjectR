// Copyright ProjectR. All Rights Reserved.

#include "PRGameplayAbility_SoldierArmoredHammerSwing02.h"

#include "ProjectR/AbilitySystem/Abilities/Enemy/SoldierArmored/PRSoldierArmoredAbilityTagUtils.h"
#include "ProjectR/PRGameplayTags.h"

UPRGameplayAbility_SoldierArmoredHammerSwing02::UPRGameplayAbility_SoldierArmoredHammerSwing02()
{
	SetAssetTags(PRSoldierArmoredAbility::MakePatternAssetTags(PRGameplayTags::Ability_Enemy_SoldierArmored_HammerSwing02));
	AbilityTag = PRGameplayTags::Ability_Enemy_SoldierArmored_HammerSwing02;

	// BP 자식이 아직 없을 때도 기본 전투 테스트가 가능하도록 둔 임시 기본값이다.
	Damage = 22.0f;
	GroggyDamage = 20.0f;
	WindupTime = 0.3f;
	RecoveryTime = 0.4f;
	AttackRange = 240.0f;
	AttackRadius = 85.0f;
}

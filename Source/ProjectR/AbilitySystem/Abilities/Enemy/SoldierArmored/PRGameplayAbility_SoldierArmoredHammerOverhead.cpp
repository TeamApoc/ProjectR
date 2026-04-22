// Copyright ProjectR. All Rights Reserved.

#include "PRGameplayAbility_SoldierArmoredHammerOverhead.h"

#include "ProjectR/AbilitySystem/Abilities/Enemy/SoldierArmored/PRSoldierArmoredAbilityTagUtils.h"
#include "ProjectR/PRGameplayTags.h"

UPRGameplayAbility_SoldierArmoredHammerOverhead::UPRGameplayAbility_SoldierArmoredHammerOverhead()
{
	SetAssetTags(PRSoldierArmoredAbility::MakePatternAssetTags(PRGameplayTags::Ability_Enemy_SoldierArmored_HammerOverhead));
	AbilityTag = PRGameplayTags::Ability_Enemy_SoldierArmored_HammerOverhead;

	// BP 자식이 아직 없을 때도 기본 전투 테스트가 가능하도록 둔 임시 기본값이다.
	Damage = 35.0f;
	GroggyDamage = 30.0f;
	WindupTime = 0.55f;
	RecoveryTime = 0.65f;
	AttackRange = 260.0f;
	AttackRadius = 95.0f;
}

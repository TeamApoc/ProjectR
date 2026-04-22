// Copyright ProjectR. All Rights Reserved.

#include "PRGameplayAbility_SoldierArmoredChargeThrust.h"

#include "ProjectR/AbilitySystem/Abilities/Enemy/SoldierArmored/PRSoldierArmoredAbilityTagUtils.h"
#include "ProjectR/PRGameplayTags.h"

UPRGameplayAbility_SoldierArmoredChargeThrust::UPRGameplayAbility_SoldierArmoredChargeThrust()
{
	SetAssetTags(PRSoldierArmoredAbility::MakePatternAssetTags(PRGameplayTags::Ability_Enemy_SoldierArmored_ChargeThrust));
	AbilityTag = PRGameplayTags::Ability_Enemy_SoldierArmored_ChargeThrust;

	// BP 자식이 아직 없을 때도 기본 전투 테스트가 가능하도록 둔 임시 기본값이다.
	Damage = 28.0f;
	GroggyDamage = 24.0f;
	WindupTime = 0.35f;
	RecoveryTime = 0.55f;
	AttackRange = 520.0f;
	AttackRadius = 90.0f;

	// Soldier_Armored 공격 애니메이션은 루트모션 기준이므로 C++ 강제 이동은 사용하지 않는다.
	bUseForwardLunge = false;
	LungeDistance = 0.0f;
}

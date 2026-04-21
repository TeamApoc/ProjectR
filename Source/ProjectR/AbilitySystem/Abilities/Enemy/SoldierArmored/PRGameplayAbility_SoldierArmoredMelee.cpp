// Copyright ProjectR. All Rights Reserved.

#include "PRGameplayAbility_SoldierArmoredMelee.h"

#include "ProjectR/PRGameplayTags.h"

namespace
{
	FGameplayTagContainer MakeSoldierAbilityTags(const FGameplayTag& PatternTag)
	{
		// 공통 Soldier_Armored 패턴 태그와 개별 공격 태그를 함께 넣어
		// 그로기/사망 시 패턴 전체를 한 번에 취소할 수 있게 한다.
		FGameplayTagContainer AssetTags;
		AssetTags.AddTag(PRGameplayTags::Ability_Enemy_SoldierArmored_Pattern);
		AssetTags.AddTag(PatternTag);
		return AssetTags;
	}
}

UPRGameplayAbility_SoldierArmoredHammerSwing01::UPRGameplayAbility_SoldierArmoredHammerSwing01()
{
	SetAssetTags(MakeSoldierAbilityTags(PRGameplayTags::Ability_Enemy_SoldierArmored_HammerSwing01));
	AbilityTag = PRGameplayTags::Ability_Enemy_SoldierArmored_HammerSwing01;
	Damage = 18.0f;
	GroggyDamage = 16.0f;
	WindupTime = 0.25f;
	RecoveryTime = 0.35f;
	AttackRange = 220.0f;
	AttackRadius = 80.0f;
}

UPRGameplayAbility_SoldierArmoredHammerSwing02::UPRGameplayAbility_SoldierArmoredHammerSwing02()
{
	SetAssetTags(MakeSoldierAbilityTags(PRGameplayTags::Ability_Enemy_SoldierArmored_HammerSwing02));
	AbilityTag = PRGameplayTags::Ability_Enemy_SoldierArmored_HammerSwing02;
	Damage = 22.0f;
	GroggyDamage = 20.0f;
	WindupTime = 0.3f;
	RecoveryTime = 0.4f;
	AttackRange = 240.0f;
	AttackRadius = 85.0f;
}

UPRGameplayAbility_SoldierArmoredHammerOverhead::UPRGameplayAbility_SoldierArmoredHammerOverhead()
{
	SetAssetTags(MakeSoldierAbilityTags(PRGameplayTags::Ability_Enemy_SoldierArmored_HammerOverhead));
	AbilityTag = PRGameplayTags::Ability_Enemy_SoldierArmored_HammerOverhead;
	Damage = 35.0f;
	GroggyDamage = 30.0f;
	WindupTime = 0.55f;
	RecoveryTime = 0.65f;
	AttackRange = 260.0f;
	AttackRadius = 95.0f;
}

UPRGameplayAbility_SoldierArmoredChargeThrust::UPRGameplayAbility_SoldierArmoredChargeThrust()
{
	SetAssetTags(MakeSoldierAbilityTags(PRGameplayTags::Ability_Enemy_SoldierArmored_ChargeThrust));
	AbilityTag = PRGameplayTags::Ability_Enemy_SoldierArmored_ChargeThrust;
	Damage = 28.0f;
	GroggyDamage = 24.0f;
	WindupTime = 0.35f;
	RecoveryTime = 0.55f;
	AttackRange = 520.0f;
	AttackRadius = 90.0f;
	// Soldier_Armored 공격 애니메이션은 루트모션 기반이므로 C++ 강제 이동은 사용하지 않는다.
	bUseForwardLunge = false;
	LungeDistance = 0.0f;
}

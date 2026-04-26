// Copyright ProjectR. All Rights Reserved.

#include "PRGameplayAbility_SoldierArmoredSprintHammer.h"

#include "ProjectR/AI/Data/PRSoldierArmoredCombatDataAsset.h"
#include "ProjectR/AbilitySystem/Abilities/Enemy/SoldierArmored/PRSoldierArmoredAbilityTagUtils.h"
#include "ProjectR/AbilitySystem/Abilities/Enemy/SoldierArmored/PRSoldierArmoredGameplayTags.h"

UPRGameplayAbility_SoldierArmoredSprintHammer::UPRGameplayAbility_SoldierArmoredSprintHammer()
{
	SetAssetTags(PRSoldierArmoredAbility::MakePatternAssetTags(PRSoldierArmoredGameplayTags::Ability_Enemy_SoldierArmored_SprintHammer));
	AbilityTag = PRSoldierArmoredGameplayTags::Ability_Enemy_SoldierArmored_SprintHammer;
}

void UPRGameplayAbility_SoldierArmoredSprintHammer::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!IsValid(CombatDataAsset))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 타이밍/몽타주는 Ability BP, 타격 수치는 공용 전투 DataAsset 책임
	const FPRSoldierArmoredAttackHitConfig& HitConfig = CombatDataAsset->SprintHammerHitConfig;
	Damage = HitConfig.Damage;
	GroggyDamage = HitConfig.GroggyDamage;
	AttackRange = HitConfig.AttackRange;
	AttackRadius = HitConfig.AttackRadius;
	AttackTraceSourceName = HitConfig.AttackTraceSourceName;
	AttackTraceSourceOffset = HitConfig.AttackTraceSourceOffset;

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

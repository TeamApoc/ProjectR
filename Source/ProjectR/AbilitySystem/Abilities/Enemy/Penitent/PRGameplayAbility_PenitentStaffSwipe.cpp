// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#include "PRGameplayAbility_PenitentStaffSwipe.h"

#include "ProjectR/PRGameplayTags.h"

UPRGameplayAbility_PenitentStaffSwipe::UPRGameplayAbility_PenitentStaffSwipe()
{
	FGameplayTagContainer PenitentStaffSwipeTags;
	PenitentStaffSwipeTags.AddTag(PRGameplayTags::Ability_Enemy_Pattern);
	PenitentStaffSwipeTags.AddTag(PRGameplayTags::Ability_Enemy_Penitent_Pattern);
	PenitentStaffSwipeTags.AddTag(PRGameplayTags::Ability_Enemy_Penitent_StaffSwipe);
	SetAssetTags(PenitentStaffSwipeTags);
	AbilityTag = PRGameplayTags::Ability_Enemy_Penitent_StaffSwipe;
}

void UPRGameplayAbility_PenitentStaffSwipe::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	const bool bUseTwoHit = IsValid(TwoHitAttackMontage) && FMath::FRand() < TwoHitChance;
	if (bUseTwoHit)
	{
		// 2타 몽타주 선택
		AttackMontage = TwoHitAttackMontage;
	}
	else if (IsValid(OneHitAttackMontage))
	{
		// 1타 몽타주 선택
		AttackMontage = OneHitAttackMontage;
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

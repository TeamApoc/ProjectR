// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRGA_FireSingle.h"

#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/Weapon/Data/PRWeaponDataAsset.h"

UPRGA_FireSingle::UPRGA_FireSingle()
{
	FGameplayTagContainer DefaultAbilityTags;
	DefaultAbilityTags.AddTag(PRGameplayTags::Ability_Player_Weapon_Fire_Primary);
	SetAssetTags(DefaultAbilityTags);

	ActivationBlockedTags.AddTag(PRGameplayTags::State_PlayerInputLocked);
}

void UPRGA_FireSingle::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	FireHitScan();
	EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/false);
}

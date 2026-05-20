// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRGameplayAbility_PenitentFireball.h"

#include "ProjectR/PRGameplayTags.h"

UPRGameplayAbility_PenitentFireball::UPRGameplayAbility_PenitentFireball()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(PRGameplayTags::Ability_Enemy_Pattern);
	AssetTags.AddTag(PRGameplayTags::Ability_Enemy_Penitent_Pattern);
	AssetTags.AddTag(PRGameplayTags::Ability_Enemy_Penitent_Fireball);
	SetAssetTags(AssetTags);

	AbilityTag = PRGameplayTags::Ability_Enemy_Penitent_Fireball;
	ProjectileSpeedOverride = 2600.0f;
	ProjectileTargetLead = 6.0f;
	DamageMultiplier = 1.0f;
	PoiseDamage = 12.0f;
	WindupTime = 0.45f;
	RecoveryTime = 0.45f;
}

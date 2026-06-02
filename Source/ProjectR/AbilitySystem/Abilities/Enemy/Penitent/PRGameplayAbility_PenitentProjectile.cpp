// Copyright ProjectR. All Rights Reserved.

#include "PRGameplayAbility_PenitentProjectile.h"

#include "ProjectR/PRGameplayTags.h"

UPRGameplayAbility_PenitentProjectile::UPRGameplayAbility_PenitentProjectile()
{
	FGameplayTagContainer PenitentProjectileTags;
	PenitentProjectileTags.AddTag(PRGameplayTags::Ability_Enemy_Pattern);
	PenitentProjectileTags.AddTag(PRGameplayTags::Ability_Enemy_Penitent_Pattern);
	PenitentProjectileTags.AddTag(PRGameplayTags::Ability_Enemy_Penitent_Projectile);
	SetAssetTags(PenitentProjectileTags);
	AbilityTag = PRGameplayTags::Ability_Enemy_Penitent_Projectile;
}

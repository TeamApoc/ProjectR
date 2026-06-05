// Copyright ProjectR. All Rights Reserved.

#include "PRGameplayAbility_FaeRoyalArcherShoot.h"

#include "ProjectR/PRGameplayTags.h"

UPRGameplayAbility_FaeRoyalArcherShoot::UPRGameplayAbility_FaeRoyalArcherShoot()
{
	FGameplayTagContainer RoyalArcherShootTags;
	RoyalArcherShootTags.AddTag(PRGameplayTags::Ability_Enemy_Pattern);
	RoyalArcherShootTags.AddTag(PRGameplayTags::Ability_Enemy_RoyalArcher_Pattern);
	RoyalArcherShootTags.AddTag(PRGameplayTags::Ability_Enemy_RoyalArcher_FlameArrow);
	SetAssetTags(RoyalArcherShootTags);

	AbilityTag = PRGameplayTags::Ability_Enemy_RoyalArcher_FlameArrow;
	ProjectileSpawnSocketName = TEXT("Bone_FA_Weapon_Arrow");
	ProjectileSpawnOffset = FVector(80.0f, 0.0f, 120.0f);
	ProjectileTargetLead = 6.0f;
	WindupTime = 0.45f;
	RecoveryTime = 0.55f;
}

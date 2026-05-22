// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "NativeGameplayTags.h"

// 성장 시스템 전용 GameplayTag 모음
namespace PRGrowthGameplayTags
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Trait_MaxHealth);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Trait_Armor);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Trait_MovementSpeed);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Trait_PlayerAttackPower);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Trait_MaxStamina);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Trait_CriticalHitChance);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Trait_CriticalDamageMultiplier);
}

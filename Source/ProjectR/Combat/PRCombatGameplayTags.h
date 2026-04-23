// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "NativeGameplayTags.h"

// 전투 파이프라인 내부 GameplayTag 모음
// 전투 전용 SetByCaller/Event Native Tag 등록 위치
namespace PRCombatGameplayTags
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Damage);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_GroggyDamage);

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Ability_EnemyMeleeHit);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Ability_EnemyComboWindow);
}

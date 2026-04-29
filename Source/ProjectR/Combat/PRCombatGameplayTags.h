// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "NativeGameplayTags.h"

// 전투 파이프라인 내부 GameplayTag 모음
// 전투 전용 SetByCaller/Event Native Tag 등록 위치
namespace PRCombatGameplayTags
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Damage); // 모드 데미지에 활용
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_GroggyDamage); // 모드 그로기 데미지에 활용
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_AttackMultiplier); // 적 어빌리티 공격 배수

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Ability_EnemyMeleeHit);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Ability_EnemyMeleeWindowBegin);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Ability_EnemyMeleeWindowTick);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Ability_EnemyMeleeWindowEnd);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Ability_EnemyComboWindow);
}

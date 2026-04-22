// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "NativeGameplayTags.h"

// 전투 파이프라인 내부에서 쓰는 GameplayTag 모음이다.
// 기존 공용 태그 파일을 건드리지 않고, 전투 전용 SetByCaller/Event 키를 네이티브 태그로 등록한다.
namespace PRCombatGameplayTags
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Damage);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_GroggyDamage);

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Ability_EnemyMeleeHit);
}

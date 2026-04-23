// Copyright ProjectR. All Rights Reserved.

#include "PRCombatGameplayTags.h"

namespace PRCombatGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Damage, "Data.Damage", "피해 SetByCaller 키");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_GroggyDamage, "Data.GroggyDamage", "그로기 피해 SetByCaller 키");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Ability_EnemyMeleeHit, "Event.Ability.EnemyMeleeHit", "적 근접 공격 타격 프레임 이벤트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Ability_EnemyComboWindow, "Event.Ability.EnemyComboWindow", "적 콤보 분기 윈도우 이벤트");
}

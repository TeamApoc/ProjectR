// Copyright ProjectR. All Rights Reserved.

#include "PRCombatGameplayTags.h"

namespace PRCombatGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Damage, "SetByCaller.Damage", "피해 SetByCaller 키");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_GroggyDamage, "SetByCaller.GroggyDamage", "그로기 피해 SetByCaller 키");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_AttackMultiplier, "SetByCaller.GroggyDamage", "그로기 피해 SetByCaller 키");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Ability_EnemyMeleeHit, "Event.Ability.EnemyMeleeHit", "적 근접 공격 타격 프레임 이벤트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Ability_EnemyMeleeWindowBegin, "Event.Ability.EnemyMeleeWindowBegin", "적 근접 공격 판정 구간 시작 이벤트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Ability_EnemyMeleeWindowTick, "Event.Ability.EnemyMeleeWindowTick", "적 근접 공격 판정 구간 갱신 이벤트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Ability_EnemyMeleeWindowEnd, "Event.Ability.EnemyMeleeWindowEnd", "적 근접 공격 판정 구간 종료 이벤트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Ability_EnemyComboWindow, "Event.Ability.EnemyComboWindow", "적 콤보 분기 윈도우 이벤트");
}

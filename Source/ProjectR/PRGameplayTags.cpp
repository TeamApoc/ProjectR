// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#include "PRGameplayTags.h"

namespace PRGameplayTags
{
	// ===== System =====
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(System_Test, "System.Test", "시스템 테스트용 태그");

	// ===== Player =====
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Player, "Player", "플레이어 관련 태그");

	// ===== Ability.* =====

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Player_Weapon_Fire_Primary, "Ability.Player.Weapon.Fire.Primary",
	                               "플레이어 주무기 발사 어빌리티");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Player_Dodge, "Ability.Player.Dodge", "플레이어 구르기 어빌리티");

	// ===== State.* =====

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dead, "State.Dead", "사망 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Groggy, "State.Groggy", "그로기 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_StaminaDepleted, "State.StaminaDepleted", "스태미너 고갈 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Invulnerable, "State.Invulnerable", "무적 상태 (회피 i-frame 등)");

	// ===== Cooldown.Ability.* =====

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Ability_Dodge, "Cooldown.Ability.Dodge", "구르기 쿨다운");

	// ===== Event.* =====

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Hit, "Event.Hit", "피격 이벤트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Ability_Death, "Event.Ability.Death", "사망 이벤트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Ability_GroggyEntered, "Event.Ability.GroggyEntered", "그로기 진입 이벤트");

	// ===== Input.Ability.* =====

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Ability_Fire_Primary, "Input.Ability.Fire.Primary", "주무기 발사 입력 태그");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Ability_Dodge, "Input.Ability.Dodge", "구르기 입력 태그");

	// ===== Fail.* =====

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Fail_Cost, "Fail.Cost", "Cost GE 자원 부족");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Fail_Cooldown, "Fail.Cooldown", "쿨다운 중");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Fail_Blocked, "Fail.Blocked", "Block 태그에 의해 차단");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Fail_Invalid, "Fail.Invalid", "어빌리티/소유자 상태 유효하지 않음");
}

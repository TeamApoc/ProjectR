// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once
#include "NativeGameplayTags.h"

// 프로젝트 전용 Native GameplayTag 선언 모음. 태그 추가 시 이 네임스페이스 안에 선언.
namespace PRGameplayTags
{
	// ===== System =====
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(System_Test); // 시스템 테스트용

	// ===== Player =====
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Player); // 플레이어 관련 루트

	// ===== Ability.* — 어빌리티 식별 =====
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Player_Weapon_Fire_Primary); // 플레이어 주무기 발사 어빌리티
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Player_Dodge); // 플레이어 구르기(회피) 어빌리티

	// ===== State.* — 캐릭터 상태 =====
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Dead); // 사망 상태.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Groggy); // 그로기 상태.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_StaminaDepleted); // 스태미너 고갈 상태.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Invulnerable); // 무적 상태.

	// ===== Cooldown.Ability.* — 쿨다운 GE가 부여 =====
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Ability_Dodge); // 구르기 쿨다운

	// ===== Event.* — SendGameplayEventToActor 이벤트 키 =====
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Hit); // 피격 이벤트.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Ability_Death); // 사망 이벤트.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Ability_GroggyEntered); // 그로기 진입 이벤트.

	// ===== Input.Ability.* — 플레이어 InputTag (AbilitySpec DynamicTags 매칭 키) =====
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Ability_Fire_Primary); // 주무기 발사 입력 태그
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Ability_Dodge); // 구르기 입력 태그

	// ===== Fail.* — CanActivateAbility 실패 사유 =====
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Fail_Cost); // Cost GE가 요구 자원 부족
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Fail_Cooldown); // Cooldown 태그가 활성 중
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Fail_Blocked); // Block 태그에 의해 차단됨
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Fail_Invalid); // 어빌리티/소유자 상태가 유효하지 않음
}

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

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_Pattern); // 일반 적 패턴 공통 루트

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_Penitent_Pattern); // Penitent 패턴 루트
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_Penitent_Fireball); // Penitent 기본 화염구
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_Penitent_BounceVolley); // Penitent 강패턴
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_Penitent_StaffSwipe); // Penitent 근접 반격

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_RoyalArcher_Pattern); // RoyalArcher 패턴 루트
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_RoyalArcher_WakeFromPerch); // RoyalArcher 전투 시작
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_RoyalArcher_FlameArrow); // RoyalArcher 주력 사격

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_SoldierArmored_Pattern); // Soldier_Armored 패턴 루트
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_SoldierArmored_HammerSwing01); // Soldier_Armored 1타
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_SoldierArmored_HammerSwing02); // Soldier_Armored 2타
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_SoldierArmored_HammerOverhead); // Soldier_Armored 강패턴
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_SoldierArmored_ChargeThrust); // Soldier_Armored 돌진

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Boss_Faerin_Pattern); // Faerin 전투 패턴 루트
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Boss_Faerin_PhaseTransition); // Faerin 페이즈 전환
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Boss_Faerin_MeleeQuickCombo); // Faerin 빠른 근접 콤보
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Boss_Faerin_PortalPairSequence); // Faerin 포털 연계
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Boss_Faerin_TeleportDash); // Faerin 텔레포트 대시
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Boss_Faerin_EnergyRain); // Faerin 범위 압박

	// ===== State.* — 캐릭터 상태 =====
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Dead); // 사망 상태.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Groggy); // 그로기 상태.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_StaminaDepleted); // 스태미너 고갈 상태.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Invulnerable); // 무적 상태.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Channeling); // 캐스팅 또는 채널링 상태.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_PhaseTransitioning); // 보스 페이즈 전환 상태.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Boss_WeakpointOpen_Core); // 보스 코어 약점 오픈 상태.

	// ===== Cooldown.Ability.* — 쿨다운 GE가 부여 =====
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Ability_Dodge); // 구르기 쿨다운

	// ===== Event.* — SendGameplayEventToActor 이벤트 키 =====
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Hit); // 피격 이벤트.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Ability_Death); // 사망 이벤트.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Ability_GroggyEntered); // 그로기 진입 이벤트.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Ability_PhaseTransition); // 페이즈 전환 이벤트.

	// ===== Cue.* — GameplayCue 식별 =====
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cue_Critical_Boss_PhaseTransition); // 보스 페이즈 전환 연출 Cue
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cue_Critical_Boss_WeakpointOpen); // 보스 약점 창 오픈 연출 Cue

	// ===== Input.Ability.* — 플레이어 InputTag (AbilitySpec DynamicTags 매칭 키) =====
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Ability_Fire_Primary); // 주무기 발사 입력 태그
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Ability_Dodge); // 구르기 입력 태그

	// ===== Fail.* — CanActivateAbility 실패 사유 =====
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Fail_Cost); // Cost GE가 요구 자원 부족
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Fail_Cooldown); // Cooldown 태그가 활성 중
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Fail_Blocked); // Block 태그에 의해 차단됨
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Fail_Invalid); // 어빌리티/소유자 상태가 유효하지 않음
}

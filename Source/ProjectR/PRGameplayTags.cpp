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
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Player_Crouch, "Ability.Player.Crouch", "플레이어 에임 어빌리티");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Player_Aim, "Ability.Player.Aim", "플레이어 구르기 어빌리티");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Player_Sprint, "Ability.Player.Sprint", "플레이어 질주 어빌리티");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Enemy_Pattern, "Ability.Enemy.Pattern", "일반 적 패턴 공통 루트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Boss_Pattern, "Ability.Boss.Pattern", "보스 패턴 공통 루트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Boss_PhaseTransition, "Ability.Boss.PhaseTransition", "보스 페이즈 전환 공통 루트");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Enemy_Penitent_Pattern, "Ability.Enemy.Penitent.Pattern", "Penitent 패턴 루트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Enemy_Penitent_Fireball, "Ability.Enemy.Penitent.Fireball", "Penitent 기본 화염구");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Enemy_Penitent_BounceVolley, "Ability.Enemy.Penitent.BounceVolley", "Penitent 강패턴");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Enemy_Penitent_StaffSwipe, "Ability.Enemy.Penitent.StaffSwipe", "Penitent 근접 반격");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Enemy_RoyalArcher_Pattern, "Ability.Enemy.RoyalArcher.Pattern", "RoyalArcher 패턴 루트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Enemy_RoyalArcher_WakeFromPerch, "Ability.Enemy.RoyalArcher.WakeFromPerch", "RoyalArcher 전투 시작");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Enemy_RoyalArcher_FlameArrow, "Ability.Enemy.RoyalArcher.FlameArrow", "RoyalArcher 주력 사격");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Enemy_SoldierArmored_Pattern, "Ability.Enemy.SoldierArmored.Pattern", "Soldier_Armored 패턴 루트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Enemy_SoldierArmored_HammerSwing01, "Ability.Enemy.SoldierArmored.HammerSwing01", "Soldier_Armored 1타");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Enemy_SoldierArmored_HammerSwing02, "Ability.Enemy.SoldierArmored.HammerSwing02", "Soldier_Armored 2타");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Enemy_SoldierArmored_HammerOverhead, "Ability.Enemy.SoldierArmored.HammerOverhead", "Soldier_Armored 강패턴");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Enemy_SoldierArmored_ChargeThrust, "Ability.Enemy.SoldierArmored.ChargeThrust", "Soldier_Armored 돌진");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Boss_Faerin_Pattern, "Ability.Boss.Faerin.Pattern", "Faerin 전투 패턴 루트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Boss_Faerin_PhaseTransition, "Ability.Boss.Faerin.PhaseTransition", "Faerin 페이즈 전환");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Boss_Faerin_MeleeQuickCombo, "Ability.Boss.Faerin.MeleeQuickCombo", "Faerin 빠른 근접 콤보");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Boss_Faerin_PortalPairSequence, "Ability.Boss.Faerin.PortalPairSequence", "Faerin 포털 연계");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Boss_Faerin_TeleportDash, "Ability.Boss.Faerin.TeleportDash", "Faerin 텔레포트 대시");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Boss_Faerin_EnergyRain, "Ability.Boss.Faerin.EnergyRain", "Faerin 범위 압박");

	// ===== State.* =====

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dead, "State.Dead", "사망 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Groggy, "State.Groggy", "그로기 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_StaminaDepleted, "State.StaminaDepleted", "스태미너 고갈 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Invulnerable, "State.Invulnerable", "무적 상태 (회피 i-frame 등)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dodging, "State.Dodging", "구르기 중 (무적이 아님)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Crouching, "State.Crouching", "앉기 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Aiming, "State.Aiming", "에이밍 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Sprinting, "State.Sprinting", "질주 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Channeling, "State.Channeling", "캐스팅 또는 채널링 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_PhaseTransitioning, "State.PhaseTransitioning", "보스 페이즈 전환 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Boss_WeakpointOpen_Core, "State.Boss.WeakpointOpen.Core", "보스 코어 약점 오픈 상태");

	// ===== Cooldown.Ability.* =====

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Ability_Dodge, "Cooldown.Ability.Dodge", "구르기 쿨다운");

	// ===== Event.* =====

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Hit, "Event.Hit", "피격 이벤트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Ability_Death, "Event.Ability.Death", "사망 이벤트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Ability_GroggyEntered, "Event.Ability.GroggyEntered", "그로기 진입 이벤트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Ability_PhaseTransition, "Event.Ability.PhaseTransition", "페이즈 전환 이벤트");

	// ===== Event.Player.* =====
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Player_HitShot, "Event.Player.HitShot", "사격이 적중했을 때");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Player_Recoil, "Event.Player.Recoil", "사격 반동 발생 (FPRRecoilEventPayload 동반)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Player_Aim_Start, "Event.Player.Aim.Start", "에이밍 진입");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Player_Aim_End, "Event.Player.Aim.End", "에이밍 해제");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Player_ChangeCrosshair, "Event.Player.ChangeCrosshair", "크로스헤어 Config 교체");

	// ===== Cue.* =====

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cue_Critical_Boss_PhaseTransition, "Cue.Critical.Boss.PhaseTransition", "보스 페이즈 전환 연출 Cue");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cue_Critical_Boss_WeakpointOpen, "Cue.Critical.Boss.WeakpointOpen", "보스 약점 창 오픈 연출 Cue");

	// ===== Input.Ability.* =====

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Ability_Fire_Primary, "Input.Ability.Fire.Primary", "주무기 발사 입력 태그");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Ability_Dodge, "Input.Ability.Dodge", "구르기 입력 태그");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Ability_Interact, "Input.Ability.Interact", "상호작용용 입력 태그");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Ability_Crouch, "Input.Ability.Crouch", "앉기용 입력 태그");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Ability_Aim, "Input.Ability.Aim", "에이밍 입력 태그");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Ability_Sprint, "Input.Ability.Sprint", "질주 입력 태그");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Ability_Mod, "Input.Ability.Mod", "모드 스킬 입력 태그");

	// ===== Input.Locomotion.* =====
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Locomotion_Walk, "Input.Locomotion.Walk", "Walk 토글 입력 태그");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Locomotion_Sprint, "Input.Locomotion.Sprint", "Sprint 토글 입력 태그");
	
	// ===== Fail.* =====

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Fail_Cost, "Fail.Cost", "Cost GE 자원 부족");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Fail_Cooldown, "Fail.Cooldown", "쿨다운 중");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Fail_Blocked, "Fail.Blocked", "Block 태그에 의해 차단");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Fail_Invalid, "Fail.Invalid", "어빌리티/소유자 상태 유효하지 않음");
}

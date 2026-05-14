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
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_PoiseDamage); // 적 어빌리티 강인도 피해
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_AmmoMagnitude); // 탄약 픽업 raw 자원량 (GE 자력값)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_AmmoScale); // 무기 장착 시 슬롯 효율 단가
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_ReserveAmmoRatio); // 무기 장착 시 슬롯 보유 한도 비율
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_AmmoCost); // 사격 시 displayed 발사 수 (raw 차감 = cost × scale)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Cooldown); // 쿨다운 GE Duration SetByCaller 키
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_ModDuration); // 지속시간형 Mod 유지 시간
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_ModMaxGauge); // 지속시간형 Mod 게이지 총 소모량

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Ability_EnemyMeleeHit);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Ability_EnemyMeleeWindowBegin);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Ability_EnemyMeleeWindowTick);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Ability_EnemyMeleeWindowEnd);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Ability_EnemyComboWindow);
}

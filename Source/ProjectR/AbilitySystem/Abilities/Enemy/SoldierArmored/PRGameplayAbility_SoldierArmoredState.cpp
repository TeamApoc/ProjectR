// Copyright ProjectR. All Rights Reserved.

#include "PRGameplayAbility_SoldierArmoredState.h"

UPRGameplayAbility_SoldierArmoredGroggy::UPRGameplayAbility_SoldierArmoredGroggy()
{
	// 기본 그로기는 몽타주 길이와 최소 유지 시간 중 더 긴 쪽을 따른다.
	GroggyDuration = 2.0f;
	bEndWhenMontageEnds = true;
}

UPRGameplayAbility_SoldierArmoredDeath::UPRGameplayAbility_SoldierArmoredDeath()
{
	// 사망 후에는 이동을 완전히 막고, 상태 유지를 위해 Ability를 자동 종료하지 않는다.
	bDisableMovementOnDeath = true;
	bEndAbilityWhenMontageEnds = false;
}

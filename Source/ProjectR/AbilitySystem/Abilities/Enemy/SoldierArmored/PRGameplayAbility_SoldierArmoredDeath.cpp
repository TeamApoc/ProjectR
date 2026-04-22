// Copyright ProjectR. All Rights Reserved.

#include "PRGameplayAbility_SoldierArmoredDeath.h"

UPRGameplayAbility_SoldierArmoredDeath::UPRGameplayAbility_SoldierArmoredDeath()
{
	// 사망 이후 이동을 확실히 막고, 상태 유지가 필요하므로 기본적으로 Ability를 자동 종료하지 않는다.
	bDisableMovementOnDeath = true;
	bEndAbilityWhenMontageEnds = false;
}

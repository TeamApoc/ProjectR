// Copyright ProjectR. All Rights Reserved.

#include "PRGameplayAbility_SoldierArmoredDeath.h"

UPRGameplayAbility_SoldierArmoredDeath::UPRGameplayAbility_SoldierArmoredDeath()
{
	// 사망 후 이동 차단 및 상태 유지
	bDisableMovementOnDeath = true;
	bEndAbilityWhenMontageEnds = false;
}

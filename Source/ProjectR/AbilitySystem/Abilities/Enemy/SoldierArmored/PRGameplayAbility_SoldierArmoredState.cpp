// Copyright ProjectR. All Rights Reserved.

#include "PRGameplayAbility_SoldierArmoredState.h"

UPRGameplayAbility_SoldierArmoredGroggy::UPRGameplayAbility_SoldierArmoredGroggy()
{
	GroggyDuration = 2.0f;
	bEndWhenMontageEnds = true;
}

UPRGameplayAbility_SoldierArmoredDeath::UPRGameplayAbility_SoldierArmoredDeath()
{
	bDisableMovementOnDeath = true;
	bEndAbilityWhenMontageEnds = false;
}

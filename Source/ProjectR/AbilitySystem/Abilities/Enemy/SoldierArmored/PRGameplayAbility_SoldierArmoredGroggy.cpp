// Copyright ProjectR. All Rights Reserved.

#include "PRGameplayAbility_SoldierArmoredGroggy.h"

UPRGameplayAbility_SoldierArmoredGroggy::UPRGameplayAbility_SoldierArmoredGroggy()
{
	// BP 자식이 아직 없을 때도 그로기 테스트가 가능하도록 둔 임시 기본값이다.
	GroggyDuration = 2.0f;
	bEndWhenMontageEnds = true;
}

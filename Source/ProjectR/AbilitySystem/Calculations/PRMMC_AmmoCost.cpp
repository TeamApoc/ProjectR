// Copyright ProjectR. All Rights Reserved.

#include "PRMMC_AmmoCost.h"

#include "ProjectR/Combat/PRCombatGameplayTags.h"

UPRMMC_AmmoCost::UPRMMC_AmmoCost()
{
}

float UPRMMC_AmmoCost::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	// 미지정 시 단발 1.0으로 처리한다
	const float AmmoCost = Spec.GetSetByCallerMagnitude(PRCombatGameplayTags::SetByCaller_AmmoCost, false, 1.0f);
	return FMath::Max(AmmoCost, 0.0f);
}

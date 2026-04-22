// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Abilities/Enemy/PRGameplayAbility_EnemyMeleeAttack.h"
#include "PRGameplayAbility_SoldierArmoredHammerSwing01.generated.h"

// Soldier_Armored의 첫 번째 기본 망치 휘두르기 공격이다.
UCLASS()
class PROJECTR_API UPRGameplayAbility_SoldierArmoredHammerSwing01 : public UPRGameplayAbility_EnemyMeleeAttack
{
	GENERATED_BODY()

public:
	UPRGameplayAbility_SoldierArmoredHammerSwing01();
};

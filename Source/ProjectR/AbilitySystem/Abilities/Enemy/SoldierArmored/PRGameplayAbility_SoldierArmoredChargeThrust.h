// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Abilities/Enemy/PRGameplayAbility_EnemyMeleeAttack.h"
#include "PRGameplayAbility_SoldierArmoredChargeThrust.generated.h"

// Soldier_Armored가 중거리에서 사용하는 돌진 찌르기 공격이다.
UCLASS()
class PROJECTR_API UPRGameplayAbility_SoldierArmoredChargeThrust : public UPRGameplayAbility_EnemyMeleeAttack
{
	GENERATED_BODY()

public:
	UPRGameplayAbility_SoldierArmoredChargeThrust();
};

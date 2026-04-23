// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Abilities/Enemy/PRGameplayAbility_EnemyDeath.h"
#include "PRGameplayAbility_SoldierArmoredDeath.generated.h"

// Soldier_Armored 전용 사망 Ability 부모 클래스
UCLASS()
class PROJECTR_API UPRGameplayAbility_SoldierArmoredDeath : public UPRGameplayAbility_EnemyDeath
{
	GENERATED_BODY()

public:
	UPRGameplayAbility_SoldierArmoredDeath();
};

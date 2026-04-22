// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Abilities/Enemy/PRGameplayAbility_EnemyMeleeAttack.h"
#include "PRGameplayAbility_SoldierArmoredHammerOverhead.generated.h"

// Soldier_Armored의 내려찍기 공격이다.
UCLASS()
class PROJECTR_API UPRGameplayAbility_SoldierArmoredHammerOverhead : public UPRGameplayAbility_EnemyMeleeAttack
{
	GENERATED_BODY()

public:
	UPRGameplayAbility_SoldierArmoredHammerOverhead();
};

// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Abilities/Enemy/PRGameplayAbility_EnemyDeath.h"
#include "ProjectR/AbilitySystem/Abilities/Enemy/PRGameplayAbility_EnemyGroggy.h"
#include "PRGameplayAbility_SoldierArmoredState.generated.h"

UCLASS()
class PROJECTR_API UPRGameplayAbility_SoldierArmoredGroggy : public UPRGameplayAbility_EnemyGroggy
{
	GENERATED_BODY()

public:
	UPRGameplayAbility_SoldierArmoredGroggy();
};

UCLASS()
class PROJECTR_API UPRGameplayAbility_SoldierArmoredDeath : public UPRGameplayAbility_EnemyDeath
{
	GENERATED_BODY()

public:
	UPRGameplayAbility_SoldierArmoredDeath();
};

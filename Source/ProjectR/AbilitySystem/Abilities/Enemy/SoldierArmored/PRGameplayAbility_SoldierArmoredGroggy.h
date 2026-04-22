// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Abilities/Enemy/PRGameplayAbility_EnemyGroggy.h"
#include "PRGameplayAbility_SoldierArmoredGroggy.generated.h"

// Soldier_Armored 전용 그로기 기본값을 가진 Ability 부모 클래스다.
UCLASS()
class PROJECTR_API UPRGameplayAbility_SoldierArmoredGroggy : public UPRGameplayAbility_EnemyGroggy
{
	GENERATED_BODY()

public:
	UPRGameplayAbility_SoldierArmoredGroggy();
};

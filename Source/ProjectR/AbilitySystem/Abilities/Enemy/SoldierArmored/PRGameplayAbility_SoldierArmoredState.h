// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Abilities/Enemy/PRGameplayAbility_EnemyDeath.h"
#include "ProjectR/AbilitySystem/Abilities/Enemy/PRGameplayAbility_EnemyGroggy.h"
#include "PRGameplayAbility_SoldierArmoredState.generated.h"

// Soldier_Armored 전용 그로기 기본값을 가진 Ability 부모 클래스다.
// 실제 몽타주는 에디터의 GA_SoldierArmored_Groggy에서 지정한다.
UCLASS()
class PROJECTR_API UPRGameplayAbility_SoldierArmoredGroggy : public UPRGameplayAbility_EnemyGroggy
{
	GENERATED_BODY()

public:
	UPRGameplayAbility_SoldierArmoredGroggy();
};

// Soldier_Armored 전용 사망 기본값을 가진 Ability 부모 클래스다.
// 사망 상태는 유지되어야 하므로 기본적으로 몽타주가 끝나도 Ability를 종료하지 않는다.
UCLASS()
class PROJECTR_API UPRGameplayAbility_SoldierArmoredDeath : public UPRGameplayAbility_EnemyDeath
{
	GENERATED_BODY()

public:
	UPRGameplayAbility_SoldierArmoredDeath();
};

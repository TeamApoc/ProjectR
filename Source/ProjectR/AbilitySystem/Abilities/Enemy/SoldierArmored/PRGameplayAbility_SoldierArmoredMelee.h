// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Abilities/Enemy/PRGameplayAbility_EnemyMeleeAttack.h"
#include "PRGameplayAbility_SoldierArmoredMelee.generated.h"

// Soldier_Armored의 첫 번째 기본 횡베기 공격이다.
UCLASS()
class PROJECTR_API UPRGameplayAbility_SoldierArmoredHammerSwing01 : public UPRGameplayAbility_EnemyMeleeAttack
{
	GENERATED_BODY()

public:
	UPRGameplayAbility_SoldierArmoredHammerSwing01();
};

// Soldier_Armored의 두 번째 기본 횡베기 공격이다.
UCLASS()
class PROJECTR_API UPRGameplayAbility_SoldierArmoredHammerSwing02 : public UPRGameplayAbility_EnemyMeleeAttack
{
	GENERATED_BODY()

public:
	UPRGameplayAbility_SoldierArmoredHammerSwing02();
};

// 느리지만 피해량과 그로기 피해가 큰 내려찍기 공격이다.
UCLASS()
class PROJECTR_API UPRGameplayAbility_SoldierArmoredHammerOverhead : public UPRGameplayAbility_EnemyMeleeAttack
{
	GENERATED_BODY()

public:
	UPRGameplayAbility_SoldierArmoredHammerOverhead();
};

// 중거리에서 사용하는 돌진 찌르기/돌진 타격 공격이다.
// 실제 이동은 공격 몽타주의 루트모션을 따른다.
UCLASS()
class PROJECTR_API UPRGameplayAbility_SoldierArmoredChargeThrust : public UPRGameplayAbility_EnemyMeleeAttack
{
	GENERATED_BODY()

public:
	UPRGameplayAbility_SoldierArmoredChargeThrust();
};

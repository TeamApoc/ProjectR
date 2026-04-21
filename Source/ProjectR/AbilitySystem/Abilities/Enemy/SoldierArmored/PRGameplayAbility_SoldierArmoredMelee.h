// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Abilities/Enemy/PRGameplayAbility_EnemyMeleeAttack.h"
#include "PRGameplayAbility_SoldierArmoredMelee.generated.h"

UCLASS()
class PROJECTR_API UPRGameplayAbility_SoldierArmoredHammerSwing01 : public UPRGameplayAbility_EnemyMeleeAttack
{
	GENERATED_BODY()

public:
	UPRGameplayAbility_SoldierArmoredHammerSwing01();
};

UCLASS()
class PROJECTR_API UPRGameplayAbility_SoldierArmoredHammerSwing02 : public UPRGameplayAbility_EnemyMeleeAttack
{
	GENERATED_BODY()

public:
	UPRGameplayAbility_SoldierArmoredHammerSwing02();
};

UCLASS()
class PROJECTR_API UPRGameplayAbility_SoldierArmoredHammerOverhead : public UPRGameplayAbility_EnemyMeleeAttack
{
	GENERATED_BODY()

public:
	UPRGameplayAbility_SoldierArmoredHammerOverhead();
};

UCLASS()
class PROJECTR_API UPRGameplayAbility_SoldierArmoredChargeThrust : public UPRGameplayAbility_EnemyMeleeAttack
{
	GENERATED_BODY()

public:
	UPRGameplayAbility_SoldierArmoredChargeThrust();
};

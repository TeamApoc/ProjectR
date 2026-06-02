// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Abilities/Enemy/PRGameplayAbility_EnemyProjectileAttack.h"
#include "PRGameplayAbility_PenitentProjectile.generated.h"

// Penitent 기본 원거리 투사체 Ability
UCLASS()
class PROJECTR_API UPRGameplayAbility_PenitentProjectile : public UPRGameplayAbility_EnemyProjectileAttack
{
	GENERATED_BODY()

public:
	// Ability 태그 초기화
	UPRGameplayAbility_PenitentProjectile();
};

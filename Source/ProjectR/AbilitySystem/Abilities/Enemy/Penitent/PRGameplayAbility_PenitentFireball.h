// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Abilities/Enemy/PRGameplayAbility_EnemyProjectileAttack.h"
#include "PRGameplayAbility_PenitentFireball.generated.h"

// Penitent 기본 화염구 공격 Ability다.
UCLASS()
class PROJECTR_API UPRGameplayAbility_PenitentFireball : public UPRGameplayAbility_EnemyProjectileAttack
{
	GENERATED_BODY()

public:
	// Penitent Fireball 태그와 기본 수치를 초기화한다.
	UPRGameplayAbility_PenitentFireball();
};

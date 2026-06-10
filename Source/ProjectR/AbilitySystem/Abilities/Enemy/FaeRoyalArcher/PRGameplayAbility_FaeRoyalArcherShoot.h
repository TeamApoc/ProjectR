// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Abilities/Enemy/PRGameplayAbility_EnemyProjectileAttack.h"
#include "PRGameplayAbility_FaeRoyalArcherShoot.generated.h"

// Fae Royal Archer의 기본 단발 화살 사격 Ability다.
UCLASS()
class PROJECTR_API UPRGameplayAbility_FaeRoyalArcherShoot : public UPRGameplayAbility_EnemyProjectileAttack
{
	GENERATED_BODY()

public:
	// 기존 RoyalArcher 태그와 기본 화살 발사 소켓을 초기화한다.
	UPRGameplayAbility_FaeRoyalArcherShoot();
};

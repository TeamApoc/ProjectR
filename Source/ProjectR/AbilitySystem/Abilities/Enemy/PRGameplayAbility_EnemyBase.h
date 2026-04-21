// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/PRGameplayAbility.h"
#include "ProjectR/Combat/PRCombatTypes.h"
#include "PRGameplayAbility_EnemyBase.generated.h"

class APREnemyBaseCharacter;
class UGameplayEffect;

UCLASS(Abstract)
class PROJECTR_API UPRGameplayAbility_EnemyBase : public UPRGameplayAbility
{
	GENERATED_BODY()

public:
	UPRGameplayAbility_EnemyBase();

protected:
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Combat")
	bool ApplyDamageContext(FPRDamageContext DamageContext) const;

	UFUNCTION(BlueprintPure, Category = "ProjectR|Combat")
	APREnemyBaseCharacter* GetEnemyAvatarCharacter() const;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat")
	TSubclassOf<UGameplayEffect> DamageEffectClass;
};

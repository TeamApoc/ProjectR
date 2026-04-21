// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Abilities/Enemy/PRGameplayAbility_EnemyBase.h"
#include "PRGameplayAbility_EnemyDeath.generated.h"

UCLASS(Abstract)
class PROJECTR_API UPRGameplayAbility_EnemyDeath : public UPRGameplayAbility_EnemyBase
{
	GENERATED_BODY()

public:
	UPRGameplayAbility_EnemyDeath();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat")
	FGameplayTagContainer CancelAbilityTags;
};

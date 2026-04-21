// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Abilities/Enemy/PRGameplayAbility_EnemyBase.h"
#include "PRGameplayAbility_EnemyGroggy.generated.h"

UCLASS(Abstract)
class PROJECTR_API UPRGameplayAbility_EnemyGroggy : public UPRGameplayAbility_EnemyBase
{
	GENERATED_BODY()

public:
	UPRGameplayAbility_EnemyGroggy();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat")
	FGameplayTagContainer CancelAbilityTags;
};

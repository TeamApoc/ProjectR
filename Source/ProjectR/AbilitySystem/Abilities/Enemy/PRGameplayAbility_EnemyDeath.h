// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Abilities/Enemy/PRGameplayAbility_EnemyBase.h"
#include "PRGameplayAbility_EnemyDeath.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAnimMontage;

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

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	UFUNCTION()
	void HandleDeathMontageCompleted();

	UFUNCTION()
	void HandleDeathMontageInterrupted();

	void FinishDeath(bool bWasCancelled);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat")
	FGameplayTagContainer CancelAbilityTags;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Animation")
	TObjectPtr<UAnimMontage> DeathMontage;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Animation", meta = (ClampMin = "0.0"))
	float MontagePlayRate = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Animation")
	bool bEndAbilityWhenMontageEnds = false;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Death")
	bool bDisableMovementOnDeath = true;

private:
	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> ActiveMontageTask;

	bool bDeathFinished = false;
};

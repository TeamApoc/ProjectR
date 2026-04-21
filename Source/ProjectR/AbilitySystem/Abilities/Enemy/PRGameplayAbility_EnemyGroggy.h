// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Abilities/Enemy/PRGameplayAbility_EnemyBase.h"
#include "PRGameplayAbility_EnemyGroggy.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAnimMontage;

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
	UFUNCTION()
	void HandleGroggyMontageCompleted();

	UFUNCTION()
	void HandleGroggyMontageInterrupted();

	void FinishGroggy(bool bWasCancelled);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat")
	FGameplayTagContainer CancelAbilityTags;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Animation")
	TObjectPtr<UAnimMontage> GroggyMontage;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Animation", meta = (ClampMin = "0.0"))
	float MontagePlayRate = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Animation")
	bool bEndWhenMontageEnds = true;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Groggy", meta = (ClampMin = "0.0"))
	float GroggyDuration = 2.0f;

private:
	FTimerHandle GroggyTimerHandle;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> ActiveMontageTask;

	bool bGroggyFinished = false;
};

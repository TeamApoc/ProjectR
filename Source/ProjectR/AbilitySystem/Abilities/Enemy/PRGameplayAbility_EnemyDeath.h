// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Abilities/Enemy/PRGameplayAbility_EnemyBase.h"
#include "PRGameplayAbility_EnemyDeath.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAnimMontage;

// State.Dead 이벤트를 받아 적을 사망 상태로 고정하는 Ability다.
// 공격 취소는 GA의 CancelAbilitiesWithTag 설정을 쓰고, C++은 이동 잠금과 사망 몽타주만 처리한다.
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
	// 사망 중 재생할 몽타주다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Animation")
	TObjectPtr<UAnimMontage> DeathMontage;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Animation", meta = (ClampMin = "0.0"))
	float MontagePlayRate = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Animation")
	bool bEndAbilityWhenMontageEnds = false;

	// true면 CharacterMovement를 DisableMovement로 잠가 사망 후 움직이지 않게 한다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Death")
	bool bDisableMovementOnDeath = true;

private:
	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> ActiveMontageTask;

	// 몽타주 콜백이 여러 번 들어와도 사망 종료 처리를 한 번만 수행한다.
	bool bDeathFinished = false;
};

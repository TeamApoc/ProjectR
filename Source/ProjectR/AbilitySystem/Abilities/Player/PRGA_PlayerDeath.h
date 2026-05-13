// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/PRGameplayAbility.h"
#include "PRGA_PlayerDeath.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAnimMontage;

// 플레이어 최종 사망 이벤트를 받아 사망 상태를 확정하는 Ability다.
UCLASS()
class PROJECTR_API UPRGA_PlayerDeath : public UPRGameplayAbility
{
	GENERATED_BODY()

public:
	UPRGA_PlayerDeath();

	/*~ UGameplayAbility Interface ~*/
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
	// 사망 몽타주가 정상 종료되었을 때 몽타주 태스크 참조를 정리한다.
	UFUNCTION()
	void HandleDeathMontageCompleted();

	// 사망 몽타주가 중단되었을 때 몽타주 태스크 참조를 정리한다.
	UFUNCTION()
	void HandleDeathMontageInterrupted();

	// 최종 사망 상태 태그, 이동 정지, 몽타주 재생을 적용한다.
	void EnterDeath();

	// 최종 사망 상태 태그를 서버에서 부여하고 복제한다.
	void ApplyDeathStateTags();

	// 이 Ability가 부여한 최종 사망 상태 태그를 제거한다.
	void ClearDeathStateTags();

	// GameMode에 플레이어 최종 사망 진입을 알린다.
	void NotifyDeathToGameMode() const;

protected:
	// 일반 사망 상태에서 재생할 몽타주다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Death|Montage")
	TObjectPtr<UAnimMontage> DeathMontage;

	// 사망 몽타주 재생 속도다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Death|Montage", meta = (ClampMin = "0.1"))
	float MontagePlayRate = 1.0f;

private:
	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> ActiveMontageTask;

	// 최종 사망 상태 태그를 이 Ability가 직접 부여했는지 나타낸다.
	bool bDeathStateTagsAdded = false;
};

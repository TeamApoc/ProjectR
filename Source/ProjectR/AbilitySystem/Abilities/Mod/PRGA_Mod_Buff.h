// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "PRGA_Mod.h"
#include "PRGA_Mod_Buff.generated.h"

class UAnimMontage;
class UGameplayEffect;
struct FGameplayEffectRemovalInfo;

// 자기 자신에게 지속시간형 버프를 적용하는 Mod 어빌리티
UCLASS()
class PROJECTR_API UPRGA_Mod_Buff : public UPRGA_Mod
{
	GENERATED_BODY()

public:
	UPRGA_Mod_Buff();

public:
	/*~ UGameplayAbility Interface ~*/
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags,
		const FGameplayTagContainer* TargetTags,
		FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;
	virtual void OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;

protected:
	// 동일 버프 이펙트가 이미 활성 중인지 확인한다
	bool HasActiveBuffEffect(const FGameplayAbilityActorInfo* ActorInfo) const;

	// 자기 자신에게 버프 이펙트를 적용한다
	bool ApplyBuffEffect();

	// 버프 발동 몽타주가 있으면 재생한다
	void PlayActivationMontageIfValid();

	// 사망 또는 다운 상태 태그 변경을 감시한다
	void BindSurvivalTagEvents();

	// 사망 또는 다운 상태 태그 감시를 해제한다
	void UnbindSurvivalTagEvents();

	// 버프 이펙트 제거 감시를 등록한다
	void BindBuffRemovalEvent();

	// 버프 이펙트 제거 감시를 해제한다
	void UnbindBuffRemovalEvent();

	// 버프 이펙트가 제거되면 내부 상태를 정리한다
	void HandleBuffEffectRemoved(const FGameplayEffectRemovalInfo& RemovalInfo);

	// 사망 또는 다운 상태 진입 시 버프를 제거한다
	void HandleSurvivalTagChanged(const FGameplayTag Tag, int32 NewCount);

	// 활성 버프 이펙트를 제거한다
	void RemoveActiveBuffEffect();

private:
	UFUNCTION()
	void HandleMontageEnd();
	
	
protected:
	// 적용할 버프 이펙트
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Mod|Buff")
	TSubclassOf<UGameplayEffect> BuffEffect;

	// 버프 이펙트 레벨
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Mod|Buff", meta = (ClampMin = "0.0"))
	float BuffEffectLevel = 1.0f;

	// 버프 발동 몽타주
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Mod|Buff|Animation")
	TObjectPtr<UAnimMontage> ActivationMontage;

	// 버프 발동 몽타주 재생 속도
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Mod|Buff|Animation", meta = (ClampMin = "0.01"))
	float ActivationMontagePlayRate = 1.0f;

private:
	// 현재 적용된 버프 이펙트 핸들
	FActiveGameplayEffectHandle ActiveBuffHandle;

	// 사망 태그 이벤트 바인딩 핸들
	FDelegateHandle DeadTagEventHandle;

	// 다운 태그 이벤트 바인딩 핸들
	FDelegateHandle DownTagEventHandle;

	// 버프 이펙트 제거 이벤트 바인딩 핸들
	FDelegateHandle BuffRemovedDelegateHandle;
};

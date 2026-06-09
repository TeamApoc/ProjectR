// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "PRGA_Mod.h"
#include "PRGA_Mod_HasDuration.generated.h"

struct FGameplayEffectRemovalInfo;
struct FOnAttributeChangeData;

// 지속시간형 비용 GE의 수명에 맞춰 활성 상태를 유지하는 모드 어빌리티
UCLASS(Abstract)
class PROJECTR_API UPRGA_Mod_HasDuration : public UPRGA_Mod
{
	GENERATED_BODY()

public:
	UPRGA_Mod_HasDuration();

public:
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
	virtual void OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;

protected:
	// 지속시간형 어빌리티 시작 처리
	UFUNCTION(BlueprintNativeEvent, Category = "ProjectR|Mod|Duration")
	void OnDurationStarted();
	virtual void OnDurationStarted_Implementation();

	// 지속시간형 어빌리티 종료 처리
	UFUNCTION(BlueprintNativeEvent, Category = "ProjectR|Mod|Duration")
	void OnDurationEnded(bool bWasCancelled);
	virtual void OnDurationEnded_Implementation(bool bWasCancelled);

	// 지속시간 비용 GE 제거 처리
	void RemoveActiveDurationCost();

private:
	void BindDurationCostRemovalEvent();
	void UnbindDurationCostRemovalEvent();
	void HandleDurationCostRemoved(const FGameplayEffectRemovalInfo& RemovalInfo);
	void BindDurationGaugeChangedEvent();
	void UnbindDurationGaugeChangedEvent();
	void HandleDurationGaugeChanged(const FOnAttributeChangeData& ChangeData);

private:
	// 활성 지속시간 비용 GE 핸들
	FActiveGameplayEffectHandle ActiveDurationCostHandle;

	// 활성 지속시간 게이지 Attribute
	FGameplayAttribute ActiveDurationGaugeAttribute;

	// 지속시간 비용 GE 제거 이벤트 바인딩 핸들
	FDelegateHandle DurationCostRemovedDelegateHandle;

	// 지속시간 게이지 변경 이벤트 바인딩 핸들
	FDelegateHandle DurationGaugeChangedDelegateHandle;

	// 지속시간 비용 GE 자연 종료 여부
	bool bDurationCostRemoved = false;

	// 지속시간 시작 여부
	bool bDurationStarted = false;
};

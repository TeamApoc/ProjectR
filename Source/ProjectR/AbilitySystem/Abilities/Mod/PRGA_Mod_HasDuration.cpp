// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRGA_Mod_HasDuration.h"

#include "AbilitySystemComponent.h"

UPRGA_Mod_HasDuration::UPRGA_Mod_HasDuration()
{
	// 지속시간형 비용 정책
	SetModCostPolicy(EPRModCostPolicy::GaugeDuration);
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
}

/*~ UGameplayAbility Interface ~*/

void UPRGA_Mod_HasDuration::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	// 지속시간 비용 상태 초기화
	ActiveDurationCostHandle.Invalidate();
	ActiveDurationGaugeAttribute = FGameplayAttribute();
	DurationCostRemovedDelegateHandle.Reset();
	DurationGaugeChangedDelegateHandle.Reset();
	bDurationCostRemoved = false;
	bDurationStarted = false;

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 지속시간 비용 확정
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/true);
		return;
	}

	// 적용된 비용 GE 추적
	ActiveDurationCostHandle = GetLastAppliedModCostHandle();
	if (!ActiveDurationCostHandle.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/true);
		return;
	}

	BindDurationCostRemovalEvent();
	BindDurationGaugeChangedEvent();
	if (!DurationGaugeChangedDelegateHandle.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/true);
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (IsValid(ASC) && ActiveDurationGaugeAttribute.IsValid()
		&& ASC->GetNumericAttribute(ActiveDurationGaugeAttribute) <= KINDA_SMALL_NUMBER)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/false);
		return;
	}

	bDurationStarted = true;
	OnDurationStarted();
}

void UPRGA_Mod_HasDuration::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// 지속시간 종료 훅
	if (bDurationStarted)
	{
		OnDurationEnded(bWasCancelled);
	}

	if (!bDurationCostRemoved)
	{
		RemoveActiveDurationCost();
	}
	else
	{
		UnbindDurationGaugeChangedEvent();
		UnbindDurationCostRemovalEvent();
		ActiveDurationCostHandle.Invalidate();
		ActiveDurationGaugeAttribute = FGameplayAttribute();
	}

	bDurationCostRemoved = false;
	bDurationStarted = false;
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UPRGA_Mod_HasDuration::OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	// 어빌리티 제거 정리
	RemoveActiveDurationCost();
	UnbindDurationGaugeChangedEvent();
	Super::OnRemoveAbility(ActorInfo, Spec);
}

/*~ 지속시간 처리 ~*/

void UPRGA_Mod_HasDuration::OnDurationStarted_Implementation()
{
	// 파생 클래스 확장 지점
}

void UPRGA_Mod_HasDuration::OnDurationEnded_Implementation(bool bWasCancelled)
{
	// 파생 클래스 확장 지점
}

void UPRGA_Mod_HasDuration::RemoveActiveDurationCost()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (IsValid(ASC) && ActiveDurationCostHandle.IsValid())
	{
		UnbindDurationGaugeChangedEvent();
		UnbindDurationCostRemovalEvent();
		ASC->RemoveActiveGameplayEffect(ActiveDurationCostHandle);
	}

	ActiveDurationCostHandle.Invalidate();
	ActiveDurationGaugeAttribute = FGameplayAttribute();
	DurationCostRemovedDelegateHandle.Reset();
	DurationGaugeChangedDelegateHandle.Reset();
}

void UPRGA_Mod_HasDuration::BindDurationCostRemovalEvent()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(ASC) || !ActiveDurationCostHandle.IsValid() || DurationCostRemovedDelegateHandle.IsValid())
	{
		return;
	}

	FOnActiveGameplayEffectRemoved_Info* RemovedDelegate = ASC->OnGameplayEffectRemoved_InfoDelegate(ActiveDurationCostHandle);
	if (RemovedDelegate != nullptr)
	{
		DurationCostRemovedDelegateHandle = RemovedDelegate->AddUObject(this, &UPRGA_Mod_HasDuration::HandleDurationCostRemoved);
	}
}

void UPRGA_Mod_HasDuration::UnbindDurationCostRemovalEvent()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(ASC) || !ActiveDurationCostHandle.IsValid() || !DurationCostRemovedDelegateHandle.IsValid())
	{
		DurationCostRemovedDelegateHandle.Reset();
		return;
	}

	FOnActiveGameplayEffectRemoved_Info* RemovedDelegate = ASC->OnGameplayEffectRemoved_InfoDelegate(ActiveDurationCostHandle);
	if (RemovedDelegate != nullptr)
	{
		RemovedDelegate->Remove(DurationCostRemovedDelegateHandle);
	}

	DurationCostRemovedDelegateHandle.Reset();
}

void UPRGA_Mod_HasDuration::HandleDurationCostRemoved(const FGameplayEffectRemovalInfo& RemovalInfo)
{
	// 비용 GE 종료
	bDurationCostRemoved = true;
	UnbindDurationGaugeChangedEvent();
	ActiveDurationCostHandle.Invalidate();
	ActiveDurationGaugeAttribute = FGameplayAttribute();
	DurationCostRemovedDelegateHandle.Reset();

	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/false);
	}
}

void UPRGA_Mod_HasDuration::BindDurationGaugeChangedEvent()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(ASC) || DurationGaugeChangedDelegateHandle.IsValid())
	{
		return;
	}

	ActiveDurationGaugeAttribute = GetCurrentModGaugeAttribute(GetCurrentActorInfo());
	if (!ActiveDurationGaugeAttribute.IsValid())
	{
		return;
	}

	DurationGaugeChangedDelegateHandle = ASC->GetGameplayAttributeValueChangeDelegate(ActiveDurationGaugeAttribute)
		.AddUObject(this, &UPRGA_Mod_HasDuration::HandleDurationGaugeChanged);
}

void UPRGA_Mod_HasDuration::UnbindDurationGaugeChangedEvent()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(ASC) || !ActiveDurationGaugeAttribute.IsValid() || !DurationGaugeChangedDelegateHandle.IsValid())
	{
		DurationGaugeChangedDelegateHandle.Reset();
		return;
	}

	ASC->GetGameplayAttributeValueChangeDelegate(ActiveDurationGaugeAttribute).Remove(DurationGaugeChangedDelegateHandle);
	DurationGaugeChangedDelegateHandle.Reset();
}

void UPRGA_Mod_HasDuration::HandleDurationGaugeChanged(const FOnAttributeChangeData& ChangeData)
{
	// 게이지 소진 종료
	if (ChangeData.NewValue > KINDA_SMALL_NUMBER || !IsActive())
	{
		return;
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/false);
}

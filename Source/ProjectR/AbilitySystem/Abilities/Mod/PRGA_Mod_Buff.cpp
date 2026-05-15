// Copyright ProjectR. All Rights Reserved.

#include "PRGA_Mod_Buff.h"

#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "GameplayEffect.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "ProjectR/Combat/PRCombatGameplayTags.h"
#include "ProjectR/PRGameplayTags.h"

UPRGA_Mod_Buff::UPRGA_Mod_Buff()
{
	SetModCostPolicy(EPRModCostPolicyType::GaugeDuration);
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
}

bool UPRGA_Mod_Buff::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	if (!IsValid(BuffEffect))
	{
		if (OptionalRelevantTags)
		{
			OptionalRelevantTags->AddTag(PRGameplayTags::Fail_Invalid);
		}
		return false;
	}

	if (HasActiveBuffEffect(ActorInfo))
	{
		if (OptionalRelevantTags)
		{
			OptionalRelevantTags->AddTag(PRGameplayTags::Fail_Blocked);
		}
		return false;
	}

	return true;
}

void UPRGA_Mod_Buff::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/true);
		return;
	}
	
	// 서버는 이펙트 적용
	if (HasAuthority(&ActivationInfo))
	{
		ApplyBuffEffect();
	}
	
	// 몽타주 재생
	if (IsValid(ActivationMontage))
	{
		UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this,
			FName("PlayMontage"),
			ActivationMontage);
		MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::HandleMontageEnd);
		MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleMontageEnd);
		MontageTask->OnCompleted.AddDynamic(this, &ThisClass::HandleMontageEnd);
		MontageTask->OnCancelled.AddDynamic(this, &ThisClass::HandleMontageEnd);
	}
	else
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}
}

void UPRGA_Mod_Buff::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UPRGA_Mod_Buff::OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	RemoveActiveBuffEffect();
	Super::OnRemoveAbility(ActorInfo, Spec);
}

bool UPRGA_Mod_Buff::HasActiveBuffEffect(const FGameplayAbilityActorInfo* ActorInfo) const
{
	const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!IsValid(ASC) || !IsValid(BuffEffect))
	{
		return false;
	}

	FGameplayEffectQuery Query;
	Query.EffectDefinition = BuffEffect;
	return ASC->GetActiveEffects(Query).Num() > 0;
}

bool UPRGA_Mod_Buff::ApplyBuffEffect()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(ASC) || !IsValid(BuffEffect) || ModDuration <= 0.0f)
	{
		return false;
	}
	// 서버 권한 확인 후 실행

	const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(BuffEffect, BuffEffectLevel);
	if (!SpecHandle.IsValid())
	{
		return false;
	}

	SpecHandle.Data->SetSetByCallerMagnitude(PRCombatGameplayTags::SetByCaller_ModDuration, ModDuration);
	SpecHandle.Data->SetDuration(ModDuration, true);
	
	ActiveBuffHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	if (!ActiveBuffHandle.IsValid())
	{
		return false;
	}

	BindBuffRemovalEvent();
	BindSurvivalTagEvents();
	return true;
}

void UPRGA_Mod_Buff::PlayActivationMontageIfValid()
{
	if (!IsValid(ActivationMontage))
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(ASC))
	{
		return;
	}

	ASC->PlayMontage(
		this,
		CurrentActivationInfo,
		ActivationMontage,
		FMath::Max(ActivationMontagePlayRate, UE_SMALL_NUMBER));
}

void UPRGA_Mod_Buff::BindSurvivalTagEvents()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(ASC))
	{
		return;
	}

	if (!DeadTagEventHandle.IsValid())
	{
		DeadTagEventHandle = ASC->RegisterGameplayTagEvent(PRGameplayTags::State_Dead, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &UPRGA_Mod_Buff::HandleSurvivalTagChanged);
	}

	if (!DownTagEventHandle.IsValid())
	{
		DownTagEventHandle = ASC->RegisterGameplayTagEvent(PRGameplayTags::State_Down, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &UPRGA_Mod_Buff::HandleSurvivalTagChanged);
	}
}

void UPRGA_Mod_Buff::UnbindSurvivalTagEvents()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(ASC))
	{
		DeadTagEventHandle.Reset();
		DownTagEventHandle.Reset();
		return;
	}

	if (DeadTagEventHandle.IsValid())
	{
		ASC->UnregisterGameplayTagEvent(DeadTagEventHandle, PRGameplayTags::State_Dead, EGameplayTagEventType::NewOrRemoved);
		DeadTagEventHandle.Reset();
	}

	if (DownTagEventHandle.IsValid())
	{
		ASC->UnregisterGameplayTagEvent(DownTagEventHandle, PRGameplayTags::State_Down, EGameplayTagEventType::NewOrRemoved);
		DownTagEventHandle.Reset();
	}
}

void UPRGA_Mod_Buff::BindBuffRemovalEvent()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(ASC) || !ActiveBuffHandle.IsValid() || BuffRemovedDelegateHandle.IsValid())
	{
		return;
	}

	FOnActiveGameplayEffectRemoved_Info* RemovedDelegate = ASC->OnGameplayEffectRemoved_InfoDelegate(ActiveBuffHandle);
	if (RemovedDelegate != nullptr)
	{
		BuffRemovedDelegateHandle = RemovedDelegate->AddUObject(this, &UPRGA_Mod_Buff::HandleBuffEffectRemoved);
	}
}

void UPRGA_Mod_Buff::UnbindBuffRemovalEvent()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(ASC) || !ActiveBuffHandle.IsValid() || !BuffRemovedDelegateHandle.IsValid())
	{
		BuffRemovedDelegateHandle.Reset();
		return;
	}

	FOnActiveGameplayEffectRemoved_Info* RemovedDelegate = ASC->OnGameplayEffectRemoved_InfoDelegate(ActiveBuffHandle);
	if (RemovedDelegate != nullptr)
	{
		RemovedDelegate->Remove(BuffRemovedDelegateHandle);
	}

	BuffRemovedDelegateHandle.Reset();
}

void UPRGA_Mod_Buff::HandleBuffEffectRemoved(const FGameplayEffectRemovalInfo& RemovalInfo)
{
	ActiveBuffHandle.Invalidate();
	BuffRemovedDelegateHandle.Reset();
	UnbindSurvivalTagEvents();
}

void UPRGA_Mod_Buff::HandleSurvivalTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount <= 0)
	{
		return;
	}

	if (Tag == PRGameplayTags::State_Dead || Tag == PRGameplayTags::State_Down)
	{
		RemoveActiveBuffEffect();
	}
}

void UPRGA_Mod_Buff::RemoveActiveBuffEffect()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (IsValid(ASC) && ActiveBuffHandle.IsValid())
	{
		UnbindBuffRemovalEvent();
		ASC->RemoveActiveGameplayEffect(ActiveBuffHandle);
	}

	ActiveBuffHandle.Invalidate();
	BuffRemovedDelegateHandle.Reset();
	UnbindSurvivalTagEvents();
}

void UPRGA_Mod_Buff::HandleMontageEnd()
{
	K2_EndAbility();
}

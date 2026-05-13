// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#include "PRGA_UseConsumable.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "ProjectR/Inventory/Components/PRInventoryComponent.h"
#include "ProjectR/Inventory/Items/PRItemInstance_Consumable.h"
#include "ProjectR/PRGameplayTags.h"

UPRGA_UseConsumable::UPRGA_UseConsumable()
{
	FGameplayTagContainer DefaultAbilityTags;
	DefaultAbilityTags.AddTag(PRGameplayTags::Ability_Player_UseConsumable);
	SetAssetTags(DefaultAbilityTags);

	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
}

bool UPRGA_UseConsumable::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                              const FGameplayAbilityActorInfo* ActorInfo,
                                              const FGameplayTagContainer* SourceTags,
                                              const FGameplayTagContainer* TargetTags,
                                              FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const UPRItemInstance_Consumable* SourceConsumableItem = Cast<UPRItemInstance_Consumable>(GetSourceObject(Handle, ActorInfo));
	return IsValid(UseMontage) && IsValid(SourceConsumableItem) && SourceConsumableItem->HasAnyStack();
}

void UPRGA_UseConsumable::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                           const FGameplayAbilityActorInfo* ActorInfo,
                                           const FGameplayAbilityActivationInfo ActivationInfo,
                                           const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	bCommitted = false;
	ActiveConsumableItem = ResolveConsumableItemFromEventData(TriggerEventData);
	if (!IsValid(ActiveConsumableItem) || !ActiveConsumableItem->HasAnyStack() || !IsValid(UseMontage))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/true);
		return;
	}

	ActiveMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		UseMontage,
		FMath::Max(UseMontagePlayRate, UE_SMALL_NUMBER));

	if (!IsValid(ActiveMontageTask))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/true);
		return;
	}

	ActiveMontageTask->OnCompleted.AddDynamic(this, &UPRGA_UseConsumable::OnConsumableMontageCompleted);
	ActiveMontageTask->OnBlendOut.AddDynamic(this, &UPRGA_UseConsumable::OnConsumableMontageCompleted);
	ActiveMontageTask->OnInterrupted.AddDynamic(this, &UPRGA_UseConsumable::OnConsumableMontageInterrupted);
	ActiveMontageTask->OnCancelled.AddDynamic(this, &UPRGA_UseConsumable::OnConsumableMontageInterrupted);
	ActiveMontageTask->ReadyForActivation();

	ActiveCommitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		PRGameplayTags::Event_Player_ConsumableCommit,
		nullptr,
		/*OnlyTriggerOnce=*/true,
		/*OnlyMatchExact=*/true);

	if (IsValid(ActiveCommitEventTask))
	{
		ActiveCommitEventTask->EventReceived.AddDynamic(this, &UPRGA_UseConsumable::OnConsumableCommitEvent);
		ActiveCommitEventTask->ReadyForActivation();
	}
}

void UPRGA_UseConsumable::EndAbility(const FGameplayAbilitySpecHandle Handle,
                                      const FGameplayAbilityActorInfo* ActorInfo,
                                      const FGameplayAbilityActivationInfo ActivationInfo,
                                      bool bReplicateEndAbility,
                                      bool bWasCancelled)
{
	ActiveMontageTask = nullptr;
	ActiveCommitEventTask = nullptr;
	ActiveConsumableItem = nullptr;
	bCommitted = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UPRGA_UseConsumable::SetConsumableItem(UPRItemInstance_Consumable* InConsumableItem)
{
	ActiveConsumableItem = InConsumableItem;
}

void UPRGA_UseConsumable::OnConsumableCommitEvent(FGameplayEventData EventData)
{
	if (bCommitted)
	{
		return;
	}

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor) || !AvatarActor->HasAuthority())
	{
		return;
	}

	if (!IsValid(ActiveConsumableItem) || !ActiveConsumableItem->HasAnyStack())
	{
		return;
	}

	if (!ApplyConsumableEffect())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[ConsumableAbility][Server] 효과 적용 실패. Ability = %s | Item = %s"),
			*GetNameSafe(this),
			*GetNameSafe(ActiveConsumableItem));
		return;
	}

	if (!ConsumeActiveItem())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[ConsumableAbility][Server] 스택 소모 실패. Ability = %s | Item = %s"),
			*GetNameSafe(this),
			*GetNameSafe(ActiveConsumableItem));
		return;
	}

	bCommitted = true;
}

void UPRGA_UseConsumable::OnConsumableMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo,
		/*bReplicateEndAbility=*/true, /*bWasCancelled=*/false);
}

void UPRGA_UseConsumable::OnConsumableMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo,
		/*bReplicateEndAbility=*/true, /*bWasCancelled=*/true);
}

bool UPRGA_UseConsumable::ApplyConsumableEffect()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(ASC) || !UseEffect.IsValid())
	{
		return false;
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(ActiveConsumableItem);

	const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(UseEffect.EffectClass, UseEffect.Level, Context);
	if (!SpecHandle.IsValid() || !SpecHandle.Data.IsValid())
	{
		return false;
	}

	SpecHandle.Data->DynamicGrantedTags.AppendTags(UseEffect.DynamicTags);
	ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
	return true;
}

bool UPRGA_UseConsumable::ConsumeActiveItem()
{
	if (!IsValid(ActiveConsumableItem))
	{
		return false;
	}

	UPRInventoryComponent* InventoryComponent = ActiveConsumableItem->GetTypedOuter<UPRInventoryComponent>();
	if (!IsValid(InventoryComponent))
	{
		return false;
	}

	InventoryComponent->RequestRemoveConsumableItem(ActiveConsumableItem, 1);
	return true;
}

UPRItemInstance_Consumable* UPRGA_UseConsumable::ResolveConsumableItemFromEventData(const FGameplayEventData* TriggerEventData) const
{
	if (TriggerEventData == nullptr)
	{
		if (UPRItemInstance_Consumable* SourceConsumableItem = Cast<UPRItemInstance_Consumable>(GetCurrentSourceObject()))
		{
			return SourceConsumableItem;
		}

		return ActiveConsumableItem;
	}

	if (UPRItemInstance_Consumable* ConsumableItem = Cast<UPRItemInstance_Consumable>(const_cast<UObject*>(TriggerEventData->OptionalObject.Get())))
	{
		return ConsumableItem;
	}

	if (UPRItemInstance_Consumable* SourceConsumableItem = Cast<UPRItemInstance_Consumable>(GetCurrentSourceObject()))
	{
		return SourceConsumableItem;
	}

	return ActiveConsumableItem;
}

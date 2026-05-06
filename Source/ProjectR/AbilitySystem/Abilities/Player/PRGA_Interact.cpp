// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRGA_Interact.h"

#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/Interaction/PRInteractorComponent.h"

UPRGA_Interact::UPRGA_Interact()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	
	FGameplayTagContainer DefaultAbilityTags;
	DefaultAbilityTags.AddTag(PRGameplayTags::Ability_Player_Interaction);
	SetAssetTags(DefaultAbilityTags);
	
	InputTag = PRGameplayTags::Input_Ability_Interact;
}

void UPRGA_Interact::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                     const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                     const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	// Remote Server: 아무것도 안함 (Cancel이나 클라측의 EndAbility를 대기)
	if (!ActorInfo->IsLocallyControlledPlayer())
	{
		return;
	}
	// 로컬: 상호작용 시도
	if (UPRInteractorComponent* Interactor = GetInteractorComponent(ActorInfo->PlayerController.Get()))
	{
		if (!Interactor->HasFocus())
		{
			K2_EndAbility();
			return;
		}
		Interactor->InteractFocused(); // TODO: Focused 자동 Interaction 대신 NPC의 경우 선택지 표시
		
		if (Interactor->IsSustained())
		{
			BindToSustained(Interactor);
		}
		else if (Interactor->IsHolding())
		{
			BindToHoldFinished(Interactor);
			WaitHoldRelease(Interactor);
		}
		else
		{
			K2_EndAbility();
		}
	}
}

void UPRGA_Interact::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (ActorInfo->IsLocallyControlledPlayer())
	{
		if (UPRInteractorComponent* Interactor = GetInteractorComponent(ActorInfo->PlayerController.Get()))
		{
			if (Interactor->IsHolding())
			{
				Interactor->OnInteractionReleased();
			}
			if (Interactor->IsSustained())
			{
				Interactor->RequestEndInteract();
			}
			Interactor->OnInteractionEnd.RemoveAll(this);
		}
	}
	
	WaitHoldReleaseTask = nullptr;
	
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

UPRInteractorComponent* UPRGA_Interact::GetInteractorComponent(AController* InController) const
{
	if (!IsValid(InController))
	{
		return nullptr;
	}
	
	if (CachedInteractorComponent.IsValid())
	{
		return CachedInteractorComponent.Get();
	}
	
	CachedInteractorComponent = InController->FindComponentByClass<UPRInteractorComponent>();
	return CachedInteractorComponent.Get();
}

void UPRGA_Interact::BindToSustained(UPRInteractorComponent* Interactor)
{
	if (!IsValid(Interactor))
	{
		return;
	}
	
	Interactor->OnInteractionEnd.RemoveAll(this);
	Interactor->OnInteractionEnd.AddWeakLambda(this, [this]()
	{
		K2_EndAbility();
	});
}

void UPRGA_Interact::BindToHoldFinished(UPRInteractorComponent* Interactor)
{
	if (!IsValid(Interactor))
	{
		return;
	}
	
	Interactor->OnHoldEnd.RemoveAll(this);
	Interactor->OnHoldEnd.AddWeakLambda(this, [this]()
	{
		if (CachedInteractorComponent.IsValid())
		{
			if (CachedInteractorComponent->IsSustained())
			{
				BindToSustained(CachedInteractorComponent.Get());
			}
			else
			{
				K2_EndAbility();
			}
		}
	});
}

void UPRGA_Interact::WaitHoldRelease(UPRInteractorComponent* Interactor)
{
	if (!IsValid(Interactor))
	{
		return;
	}
	
	WaitHoldReleaseTask = UAbilityTask_WaitInputRelease::WaitInputRelease(this, false);
	WaitHoldReleaseTask->OnRelease.AddDynamic(this, &ThisClass::OnInputReleased);
	WaitHoldReleaseTask->ReadyForActivation();
}

void UPRGA_Interact::OnInputReleased(float TimeHeld)
{
	K2_EndAbility();
}

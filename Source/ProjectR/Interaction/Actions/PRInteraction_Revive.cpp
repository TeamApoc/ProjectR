// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRInteraction_Revive.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/Inventory/Components/PRInventoryComponent.h"
#include "ProjectR/Inventory/Data/PRConsumableDataAsset.h"
#include "ProjectR/Inventory/Items/PRItemInstance_Consumable.h"
#include "ProjectR/Utils/PRGameplayStatics.h"

void UPRInteraction_Revive::OnHoldStart_Implementation(AActor* Interactor)
{
	Super::OnHoldStart_Implementation(Interactor);
	
	if (UAbilitySystemComponent* ASC = UPRGameplayStatics::GetAbilitySystemComponent(Interactor))
	{
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(ASC->GetOwner(),PRGameplayTags::Event_Ability_Revive,FGameplayEventData());
	}
}

void UPRInteraction_Revive::OnHoldCanceled_Implementation(AActor* Interactor)
{
	Super::OnHoldCanceled_Implementation(Interactor);
	FinishReviveAbility(Interactor);
}

void UPRInteraction_Revive::OnHoldFinished_Implementation(AActor* Interactor)
{
	Super::OnHoldFinished_Implementation(Interactor);
	FinishReviveAbility(Interactor);
}

void UPRInteraction_Revive::Execute_Implementation(AActor* Interactor)
{
	ensureMsgf(IsValid(CostItem),TEXT("소생 코스트 아이템이 설정되어 있어야함."));
	Super::Execute_Implementation(Interactor);
	
	if (UAbilitySystemComponent* OwnerASC = UPRGameplayStatics::GetAbilitySystemComponent(GetOwner()))
	{
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(OwnerASC->GetOwner(),PRGameplayTags::Event_Ability_GetUp,FGameplayEventData());
	}
	
	if (UPRInventoryComponent* InteractorInventory = UPRGameplayStatics::GetInventoryComponent(Interactor))
	{
		InteractorInventory->RequestRemoveConsumableItemByData(CostItem,1);
	}
}

bool UPRInteraction_Revive::CanInteract_Implementation(AActor* Interactor) const
{
	ensureMsgf(IsValid(CostItem),TEXT("소생 코스트 아이템이 설정되어 있어야함."));
	
	UAbilitySystemComponent* OwnerASC = UPRGameplayStatics::GetAbilitySystemComponent(GetOwner());
	if (!IsValid(OwnerASC))
	{
		return false;
	}
	
	if (!OwnerASC->HasMatchingGameplayTag(PRGameplayTags::State_Down))
	{
		return false;
	}
	
	UPRInventoryComponent* InteractorInventory = UPRGameplayStatics::GetInventoryComponent(Interactor);
	if (!IsValid(InteractorInventory))
	{
		return false;
	}
	
	if (UPRItemInstance_Consumable* CostItemInstance = InteractorInventory->FindConsumableItemByData(CostItem))
	{
		return CostItemInstance->GetStackCount() > 0;
	}
	
	return false;
}

void UPRInteraction_Revive::FinishReviveAbility(AActor* Interactor)
{
	if (UAbilitySystemComponent* ASC = UPRGameplayStatics::GetAbilitySystemComponent(Interactor))
	{
		FGameplayTagContainer TagContainer;
		TagContainer.AddTag(PRGameplayTags::Ability_Player_Revive);
		ASC->CancelAbilities(&TagContainer);
	}
}

// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRInteraction_Revive.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "ProjectR/PRGameplayTags.h"
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
	Super::Execute_Implementation(Interactor);
	
	if (UAbilitySystemComponent* ASC = UPRGameplayStatics::GetAbilitySystemComponent(GetOwner()))
	{
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(ASC->GetOwner(),PRGameplayTags::Event_Ability_GetUp,FGameplayEventData());
	}
}

bool UPRInteraction_Revive::CanInteract_Implementation(AActor* Interactor) const
{
	if (UAbilitySystemComponent* ASC = UPRGameplayStatics::GetAbilitySystemComponent(GetOwner()))
	{
		return ASC->HasMatchingGameplayTag(PRGameplayTags::State_Down);
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

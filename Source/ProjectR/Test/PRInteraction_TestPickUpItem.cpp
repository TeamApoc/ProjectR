// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRInteraction_TestPickUpItem.h"

#include "ProjectR/Inventory/Components/PRInventoryComponent.h"
#include "ProjectR/Inventory/Data/PRItemDataAsset.h"
#include "ProjectR/Utils/PRGameplayStatics.h"

void UPRInteraction_TestPickUpItem::Execute_Implementation(AActor* Interactor)
{
	Super::Execute_Implementation(Interactor);
	
	if (!IsValid(Interactor))
	{
		return;
	}
	
	if (!IsValid(ItemData) || Amount <= 0)
	{
		return;
	}
	
	UPRInventoryComponent* InventoryComponent = UPRGameplayStatics::GetInventoryComponent(Interactor);
	if (!IsValid(InventoryComponent))
	{
		return;
	}
	
	if (InventoryComponent->AddItem(ItemData, Amount))
	{
		if (bDestroyOnPickUp)
		{
			GetOwner()->Destroy();
		}
	}
}

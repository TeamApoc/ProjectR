// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRInteraction_PickUp.h"

#include "ProjectR/Inventory/Components/PRInventoryComponent.h"
#include "ProjectR/Utils/PRGameplayStatics.h"

void UPRInteraction_PickUp::Execute_Implementation(AActor* Interactor)
{
	Super::Execute_Implementation(Interactor);
	
	UE_LOG(LogTemp,Warning,TEXT("UPRInteraction_PickUp::Execute_Implementation()"));
	if (AActor* Owner = GetOwner())
	{
		if (!Owner->HasAuthority())
		{
			return;
		}
		
		if (UPRInventoryComponent* Inventory = UPRGameplayStatics::GetInventoryComponent(Interactor))
		{
			Inventory->RequestAddWeaponItem(WeaponDataAsset);
		}
		
		Owner->Destroy();
	}
}

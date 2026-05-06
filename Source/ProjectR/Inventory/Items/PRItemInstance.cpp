// Copyright ProjectR. All Rights Reserved.

#include "PRItemInstance.h"

#include "ProjectR/Inventory/Components/PRInventoryComponent.h"

UPRItemInstance::UPRItemInstance()
{
}

bool UPRItemInstance::IsSupportedForNetworking() const
{
	return true;
}

void UPRItemInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void UPRItemInstance::NotifyInventoryChanged(EPRInventoryChangeReason ChangeReason)
{
	if (UPRInventoryComponent* InventoryComponent = GetTypedOuter<UPRInventoryComponent>())
	{
		InventoryComponent->OnInventoryChanged(ChangeReason);
	}
}

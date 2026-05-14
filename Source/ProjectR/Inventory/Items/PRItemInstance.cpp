// Copyright ProjectR. All Rights Reserved.

#include "PRItemInstance.h"

#include "Net/UnrealNetwork.h"
#include "ProjectR/Inventory/Components/PRInventoryComponent.h"
#include "ProjectR/Inventory/Data/PRItemDataAsset.h"

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
	
	DOREPLIFETIME(ThisClass, ItemData);
	DOREPLIFETIME(ThisClass, StackCount);
}

void UPRItemInstance::InitializeItem(UPRItemDataAsset* InItemData, int32 InitialStackCount)
{
	ItemData = InItemData;
	StackCount = FMath::Max(InitialStackCount, 0);

	if (IsValid(InItemData))
	{
		StackCount = FMath::Min(StackCount, InItemData->MaxStackCount);
	}
}

void UPRItemInstance::NotifyInventoryChanged(EPRInventoryChangeReason ChangeReason)
{
	if (UPRInventoryComponent* InventoryComponent = GetTypedOuter<UPRInventoryComponent>())
	{
		FPRInventoryChangeEventData EventData;
		EventData.ItemInstance = this;
		EventData.ChangeReason = ChangeReason;
		InventoryComponent->OnInventoryChanged(EventData);
	}
}

void UPRItemInstance::OnRep_StackCount(const int32& OldStackCount)
{
	// 동일한 값으로 갱신된 경우 알림을 생략한다
	if (StackCount == OldStackCount)
	{
		return;
	}

	NotifyInventoryChanged(EPRInventoryChangeReason::StackChanged);
}

bool UPRItemInstance::HasAnyStack() const
{
	return StackCount > 0;
}

bool UPRItemInstance::AddStack(int32 AddCount)
{
	if (AddCount <= 0 || !IsValid(ItemData))
	{
		return false;
	}

	const int32 PreviousStackCount = StackCount;
	StackCount = FMath::Clamp(StackCount + AddCount, 0, ItemData->MaxStackCount);

	if (StackCount == PreviousStackCount)
	{
		return false;
	}

	NotifyInventoryChanged(EPRInventoryChangeReason::StackChanged);
	return true;
}

bool UPRItemInstance::RemoveStack(int32 RemoveCount)
{
	if (RemoveCount <= 0 || StackCount < RemoveCount)
	{
		return false;
	}

	StackCount -= RemoveCount;
	NotifyInventoryChanged(EPRInventoryChangeReason::StackChanged);
	return true;
}


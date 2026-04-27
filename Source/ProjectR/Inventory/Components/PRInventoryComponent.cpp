// Copyright ProjectR. All Rights Reserved.

#include "PRInventoryComponent.h"

#include "ProjectR/Inventory/Items/PRItemInstance.h"
#include "ProjectR/Weapon/Data/PRWeaponDataAsset.h"
#include "ProjectR/Weapon/Items/PRItemInstance_Weapon.h"

UPRInventoryComponent::UPRInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

UPRItemInstance_Weapon* UPRInventoryComponent::AddWeaponItem(UPRWeaponDataAsset* WeaponData)
{
	// 무기 정적 데이터가 없으면 Item 생성 자체를 진행하지 않는다
	if (!IsValid(WeaponData))
	{
		return nullptr;
	}

	UPRItemInstance_Weapon* NewWeaponItem = NewObject<UPRItemInstance_Weapon>(this);
	if (!IsValid(NewWeaponItem))
	{
		return nullptr;
	}

	NewWeaponItem->InitializeWeaponItem(WeaponData);
	RegisterReplicatedItem(NewWeaponItem);
	return NewWeaponItem;
}

UPRItemInstance_Weapon* UPRInventoryComponent::FindWeaponByItemId(const FGuid& ItemId) const
{
	// 잘못된 식별자면 조회를 수행하지 않는다
	if (!ItemId.IsValid())
	{
		return nullptr;
	}

	for (UPRItemInstance* ItemInstance : ItemInstances)
	{
		UPRItemInstance_Weapon* WeaponItem = Cast<UPRItemInstance_Weapon>(ItemInstance);
		if (!IsValid(WeaponItem))
		{
			continue;
		}

		if (WeaponItem->GetItemId() == ItemId)
		{
			return WeaponItem;
		}
	}

	return nullptr;
}

bool UPRInventoryComponent::OwnsWeapon(const UPRItemInstance_Weapon* WeaponItem) const
{
	// 유효하지 않은 Item은 소유권 검사에서 즉시 실패한다
	if (!IsValid(WeaponItem))
	{
		return false;
	}

	return ItemInstances.Contains(WeaponItem);
}

void UPRInventoryComponent::RegisterReplicatedItem(UPRItemInstance* Item)
{
	// 인벤토리에 실제로 들어갈 Item만 목록에 등록한다
	if (!IsValid(Item))
	{
		return;
	}

	ItemInstances.AddUnique(Item);
}

void UPRInventoryComponent::UnregisterReplicatedItem(UPRItemInstance* Item)
{
	// 잘못된 Item 요청이면 등록 해제를 시도하지 않는다
	if (!IsValid(Item))
	{
		return;
	}

	ItemInstances.Remove(Item);
}

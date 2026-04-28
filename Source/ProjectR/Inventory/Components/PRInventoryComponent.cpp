// Copyright ProjectR. All Rights Reserved.

#include "PRInventoryComponent.h"

#include "Engine/ActorChannel.h"
#include "Net/UnrealNetwork.h"
#include "ProjectR/Inventory/Items/PRItemInstance.h"
#include "ProjectR/Weapon/Data/PRWeaponDataAsset.h"
#include "ProjectR/Weapon/Data/PRWeaponModDataAsset.h"
#include "ProjectR/Weapon/Items/PRItemInstance_Weapon.h"

UPRInventoryComponent::UPRInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UPRInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UPRInventoryComponent, ItemInstances, COND_OwnerOnly);
}

bool UPRInventoryComponent::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool bWroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	if (!RepFlags->bNetOwner)
	{
		return bWroteSomething;
	}

	for (UPRItemInstance* ItemInstance : ItemInstances)
	{
		if (!IsValid(ItemInstance))
		{
			continue;
		}

		bWroteSomething |= Channel->ReplicateSubobject(ItemInstance, *Bunch, *RepFlags);
	}

	return bWroteSomething;
}

void UPRInventoryComponent::RequestAddWeaponItem(UPRWeaponDataAsset* WeaponData)
{
	// 데이터가 없는 요청은 서버 RPC를 보내기 전에 중단한다
	if (!IsValid(WeaponData))
	{
		return;
	}

	if (IsValid(GetOwner()) && GetOwner()->HasAuthority())
	{
		AddWeaponItem(WeaponData);
		return;
	}

	Server_RequestAddWeaponItem(WeaponData);
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

	if (IsValid(GetOwner()) && GetOwner()->HasAuthority())
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("[Inventory][Server] AddItem. Owner=%s ItemId=%s Weapon=%s WeaponId=%s Count=%d"),
			*GetNameSafe(GetOwner()),
			*NewWeaponItem->GetItemId().ToString(),
			*GetNameSafe(WeaponData),
			*WeaponData->WeaponId.ToString(),
			ItemInstances.Num());
	}

	return NewWeaponItem;
}

UPRItemInstance_Weapon* UPRInventoryComponent::GetWeaponItemAtIndex(int32 ItemIndex) const
{
	if (!ItemInstances.IsValidIndex(ItemIndex))
	{
		return nullptr;
	}

	return Cast<UPRItemInstance_Weapon>(ItemInstances[ItemIndex]);
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

void UPRInventoryComponent::OnRep_ItemInstances()
{
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[Inventory][Client] ItemInstances replicated. Owner=%s Count=%d"),
		*GetNameSafe(GetOwner()),
		ItemInstances.Num());

	for (int32 ItemIndex = 0; ItemIndex < ItemInstances.Num(); ++ItemIndex)
	{
		const UPRItemInstance_Weapon* WeaponItem = Cast<UPRItemInstance_Weapon>(ItemInstances[ItemIndex]);
		if (!IsValid(WeaponItem))
		{
			UE_LOG(
				LogTemp,
				Log,
				TEXT("[Inventory][Client] Item[%d] Item=%s"),
				ItemIndex,
				*GetNameSafe(ItemInstances[ItemIndex]));
			continue;
		}

		const UPRWeaponDataAsset* WeaponData = WeaponItem->GetWeaponData();
		UE_LOG(
			LogTemp,
			Log,
			TEXT("[Inventory][Client] Item[%d] WeaponItem. ItemId=%s Weapon=%s WeaponId=%s Mod=%s"),
			ItemIndex,
			*WeaponItem->GetItemId().ToString(),
			*GetNameSafe(WeaponData),
			IsValid(WeaponData) ? *WeaponData->WeaponId.ToString() : TEXT("None"),
			*GetNameSafe(WeaponItem->GetModData()));
	}
}

void UPRInventoryComponent::RegisterReplicatedItem(UPRItemInstance* Item)
{
	// 인벤토리에 실제로 들어갈 Item만 목록에 등록한다
	if (!IsValid(Item))
	{
		return;
	}

	ItemInstances.AddUnique(Item);

	if (IsValid(GetOwner()))
	{
		GetOwner()->ForceNetUpdate();
	}
}

void UPRInventoryComponent::UnregisterReplicatedItem(UPRItemInstance* Item)
{
	// 잘못된 Item 요청이면 등록 해제를 시도하지 않는다
	if (!IsValid(Item))
	{
		return;
	}

	ItemInstances.Remove(Item);

	if (IsValid(GetOwner()))
	{
		GetOwner()->ForceNetUpdate();
	}
}

void UPRInventoryComponent::Server_RequestAddWeaponItem_Implementation(UPRWeaponDataAsset* WeaponData)
{
	AddWeaponItem(WeaponData);
}

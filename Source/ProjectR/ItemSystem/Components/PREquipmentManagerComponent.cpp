// Copyright ProjectR. All Rights Reserved.

#include "PREquipmentManagerComponent.h"
#include "Engine/ActorChannel.h"
#include "Net/UnrealNetwork.h"
#include "ProjectR/Character/PRCharacterBase.h"
#include "ProjectR/ItemSystem/Components/PRInventoryComponent.h"
#include "ProjectR/ItemSystem/Data/PREquipmentDataAsset.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Equipment.h"
#include "ProjectR/Utils/PRGameplayStatics.h"

void FPREquipmentList::EquipItem(EPREquipmentSlotType SlotType, UPRItemInstance_Equipment* Item)
{
	for (FPREquippedItemEntry& Entry : Entries)
	{
		if (Entry.SlotType == SlotType)
		{
			Entry.EquipmentItem = Item;
			return;
		}
	}
	FPREquippedItemEntry NewEntry;
	NewEntry.SlotType = SlotType;
	NewEntry.EquipmentItem = Item;
	Entries.Add(NewEntry);
}

UPRItemInstance_Equipment* FPREquipmentList::UnequipSlot(EPREquipmentSlotType SlotType)
{
	for (int32 i = 0; i < Entries.Num(); ++i)
	{
		if (Entries[i].SlotType == SlotType)
		{
			UPRItemInstance_Equipment* Item = Entries[i].EquipmentItem;
			Entries.RemoveAt(i);
			return Item;
		}
	}
	return nullptr;
}

UPRItemInstance_Equipment* FPREquipmentList::GetEquippedItem(EPREquipmentSlotType SlotType) const
{
	for (const FPREquippedItemEntry& Entry : Entries)
	{
		if (Entry.SlotType == SlotType)
		{
			return Entry.EquipmentItem;
		}
	}
	return nullptr;
}

bool FPREquipmentList::IsSlotOccupied(EPREquipmentSlotType SlotType) const
{
	for (const FPREquippedItemEntry& Entry : Entries)
	{
		if (Entry.SlotType == SlotType)
		{
			return true;
		}
	}
	return false;
}

TArray<EPREquipmentSlotType> FPREquipmentList::GetActiveSlots() const
{
	TArray<EPREquipmentSlotType> ActiveSlots;
	ActiveSlots.Reserve(Entries.Num());
	for (const FPREquippedItemEntry& Entry : Entries)
	{
		ActiveSlots.Add(Entry.SlotType);
	}
	return ActiveSlots;
}

UPREquipmentManagerComponent::UPREquipmentManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UPREquipmentManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UPREquipmentManagerComponent, EquippedList, COND_OwnerOnly);
	DOREPLIFETIME(UPREquipmentManagerComponent, ReplicatedEquipmentInfos);
}

bool UPREquipmentManagerComponent::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool bWroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	if (!RepFlags->bNetOwner)
	{
		return bWroteSomething;
	}

	for (const FPREquippedItemEntry& Entry : EquippedList.GetEntries())
	{
		if (IsValid(Entry.EquipmentItem))
		{
			bWroteSomething |= Channel->ReplicateSubobject(Entry.EquipmentItem, *Bunch, *RepFlags);
		}
	}

	return bWroteSomething;
}

bool UPREquipmentManagerComponent::EquipItem(UPRItemInstance_Equipment* EquipmentItem)
{
	if (!IsValid(EquipmentItem) || !IsValid(EquipmentItem->GetEquipmentData()))
	{
		return false;
	}

	const EPREquipmentSlotType SlotType = EquipmentItem->GetSlotType();
	if (SlotType == EPREquipmentSlotType::None)
	{
		return false;
	}

	// 기존 장착 해제
	UnequipSlot(SlotType);

	EquippedList.EquipItem(SlotType, EquipmentItem);
	EquipmentItem->OnEquipped(GetOwner());

	// 시각 정보 업데이트
	FPRReplicatedEquipmentInfo NewEquipmentInfo;
	NewEquipmentInfo.SlotType = SlotType;
	NewEquipmentInfo.EquipmentData = EquipmentItem->GetEquipmentData();

	bool bFound = false;
	for (FPRReplicatedEquipmentInfo& Info : ReplicatedEquipmentInfos)
	{
		if (Info.SlotType == SlotType)
		{
			Info = NewEquipmentInfo;
			bFound = true;
			break;
		}
	}
	if (!bFound)
	{
		ReplicatedEquipmentInfos.Add(NewEquipmentInfo);
	}

	if (IsValid(GetOwner()))
	{
		GetOwner()->ForceNetUpdate();
	}

	OnEquipmentVisualInfosChanged.Broadcast(this);
	OnEquipmentChanged.Broadcast(SlotType, EquipmentItem);
	return true;
}

bool UPREquipmentManagerComponent::UnequipSlot(EPREquipmentSlotType SlotType)
{
	if (!IsSlotOccupied(SlotType))
	{
		return false;
	}

	UPRItemInstance_Equipment* ItemToUnequip = EquippedList.UnequipSlot(SlotType);

	if (IsValid(ItemToUnequip))
	{
		ItemToUnequip->OnUnequipped(GetOwner());
	}

	// 시각 정보 제거
	for (int32 i = ReplicatedEquipmentInfos.Num() - 1; i >= 0; --i)
	{
		if (ReplicatedEquipmentInfos[i].SlotType == SlotType)
		{
			ReplicatedEquipmentInfos.RemoveAt(i);
			break;
		}
	}

	if (IsValid(GetOwner()))
	{
		GetOwner()->ForceNetUpdate();
	}

	OnEquipmentVisualInfosChanged.Broadcast(this);
	OnEquipmentChanged.Broadcast(SlotType, nullptr);
	return true;
}

UPRItemInstance_Equipment* UPREquipmentManagerComponent::GetEquippedItem(EPREquipmentSlotType SlotType) const
{
	return EquippedList.GetEquippedItem(SlotType);
}

bool UPREquipmentManagerComponent::IsSlotOccupied(EPREquipmentSlotType SlotType) const
{
	return EquippedList.IsSlotOccupied(SlotType);
}

FPREquipmentSaveData UPREquipmentManagerComponent::MakeSaveData() const
{
	FPREquipmentSaveData SaveData;

	UPRInventoryComponent* InventoryComponent = UPRGameplayStatics::GetInventoryComponent(GetOwner());
	if (!IsValid(InventoryComponent))
	{
		return SaveData;
	}

	for (const FPREquippedItemEntry& Entry : EquippedList.GetEntries())
	{
		UPRItemInstance_Equipment* Item = Entry.EquipmentItem;
		if (IsValid(Item))
		{
			FPREquipmentSlotSaveEntry SaveEntry;
			Item->FillSaveEntry(SaveEntry);
			SaveEntry.EquipmentItemIndex = InventoryComponent->GetItemIndexByType(Item, EPRItemType::Equipment);
			SaveData.EquippedSlots.Add(SaveEntry);
		}
	}

	return SaveData;
}

void UPREquipmentManagerComponent::ApplySaveData(const FPREquipmentSaveData& InSaveData)
{
	// 기존 장착 상태 초기화
	TArray<EPREquipmentSlotType> ActiveSlots = EquippedList.GetActiveSlots();
	for (EPREquipmentSlotType SlotType : ActiveSlots)
	{
		UnequipSlot(SlotType);
	}
	
	UPRInventoryComponent* InventoryComponent = UPRGameplayStatics::GetInventoryComponent(GetOwner());
	if (!IsValid(InventoryComponent))
	{
		return;
	}

	// 인벤토리에서 복원된 아이템을 인덱스 기반으로 장착
	for (const FPREquipmentSlotSaveEntry& Entry : InSaveData.EquippedSlots)
	{
		UPRItemInstance_Equipment* ItemToEquip = InventoryComponent->GetItemAtIndexByType<UPRItemInstance_Equipment>(EPRItemType::Equipment, Entry.EquipmentItemIndex);
		if (IsValid(ItemToEquip))
		{
			ItemToEquip->ApplySaveEntry(Entry);
			EquipItem(ItemToEquip);
		}
	}
}

void UPREquipmentManagerComponent::ResetSystem()
{
	// 장착 데이터 유지와 외형 재적용 알림
	OnEquipmentVisualInfosChanged.Broadcast(this);
}

void UPREquipmentManagerComponent::OnRep_EquipmentInfos()
{
	// 클라이언트에서 시각적 장착 정보가 복제되었을 때 캐릭터 파츠 메시 갱신
	OnEquipmentVisualInfosChanged.Broadcast(this);

	// 장비 외형 복제 흐름 확인용 로그
	UE_LOG(LogTemp, Log, TEXT("[Equipment] 장비 시각 정보 복제됨. 아이템 개수: %d"), ReplicatedEquipmentInfos.Num());
}

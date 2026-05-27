// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRItemInstance_Mod.h"

#include "Logging/LogMacros.h"
#include "Net/UnrealNetwork.h"
#include "ProjectR/Game/PRGameTypes.h"
#include "ProjectR/ItemSystem/Components/PRInventoryComponent.h"
#include "ProjectR/ItemSystem/Data/PRWeaponModDataAsset.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Weapon.h"

void UPRItemInstance_Mod::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UPRItemInstance_Mod, EquippedWeaponItem);
}

void UPRItemInstance_Mod::InitializeItem(UPRItemDataAsset* InItemData, int32 InitialStackCount)
{
	Super::InitializeItem(InItemData, InitialStackCount);
	ClearEquippedWeaponItem();
}

UPRWeaponModDataAsset* UPRItemInstance_Mod::GetModData() const
{
	return Cast<UPRWeaponModDataAsset>(ItemData);
}

void UPRItemInstance_Mod::FillSaveEntry(FPRModItemSaveEntry& OutEntry) const
{
	// 저장 엔트리 기본값
	OutEntry = FPRModItemSaveEntry();
	OutEntry.ModData = GetModData();
}

void UPRItemInstance_Mod::ApplySaveEntry(const FPRModItemSaveEntry& InEntry)
{
}

bool UPRItemInstance_Mod::MatchesModData(const UPRWeaponModDataAsset* InModData) const
{
	return GetModData() == InModData;
}

bool UPRItemInstance_Mod::IsEquipped() const
{
	return IsValid(EquippedWeaponItem);
}

bool UPRItemInstance_Mod::CanEquipToWeaponItem(const UPRItemInstance_Weapon* WeaponItem) const
{
	if (!IsValid(WeaponItem))
	{
		return false;
	}

	return !IsEquipped() || EquippedWeaponItem == WeaponItem;
}

void UPRItemInstance_Mod::MarkEquippedToWeaponItem(UPRItemInstance_Weapon* WeaponItem)
{
	if (!IsValid(WeaponItem))
	{
		return;
	}

	EquippedWeaponItem = WeaponItem;
}

void UPRItemInstance_Mod::ClearEquippedWeaponItem()
{
	EquippedWeaponItem = nullptr;
}

void UPRItemInstance_Mod::OnRep_ModData()
{
	// 클라이언트에서 Mod 데이터 복제 결과를 Item 참조 기준으로 추적
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[Inventory][Client] Mod item data replicated. Item = %s | Mod = %s"),
		*GetNameSafe(this),
		*GetNameSafe(GetModData()));
}

void UPRItemInstance_Mod::OnRep_EquippedWeaponItem()
{
	// 클라이언트에서 Mod Item의 장착 대상 변경을 Item 참조 기준으로 추적
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[Inventory][Client] Mod item equipped weapon replicated. Item = %s | Mod = %s | EquippedWeaponItem = %s"),
		*GetNameSafe(this),
		*GetNameSafe(GetModData()),
		*GetNameSafe(EquippedWeaponItem.Get()));

	NotifyInventoryChanged(EPRInventoryChangeReason::ModEquipChanged);
}

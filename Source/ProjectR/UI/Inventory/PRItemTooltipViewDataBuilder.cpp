// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#include "PRItemTooltipViewDataBuilder.h"

#include "ProjectR/ItemSystem/Data/PRConsumableDataAsset.h"
#include "ProjectR/ItemSystem/Data/PREquipmentDataAsset.h"
#include "ProjectR/ItemSystem/Data/PRItemDataAsset.h"
#include "ProjectR/ItemSystem/Data/PRMaterialDataAsset.h"
#include "ProjectR/ItemSystem/Data/PRWeaponDataAsset.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Weapon.h"
#include "ProjectR/UI/Inventory/PRInventoryItemSlotViewDataBuilder.h"
#include "ProjectR/UI/PRUserInterfaceStatics.h"

namespace
{
	FText GetItemTypeText(EPRItemType ItemType)
	{
		switch (ItemType)
		{
		case EPRItemType::Weapon:
		case EPRItemType::PrimaryWeapon:
		case EPRItemType::SecondaryWeapon:
			return FText::FromString(TEXT("무기"));

		case EPRItemType::Mod:
			return FText::FromString(TEXT("Mod"));

		case EPRItemType::Consumable:
			return FText::FromString(TEXT("소비"));

		case EPRItemType::Material:
			return FText::FromString(TEXT("재료"));

		case EPRItemType::Equipment:
			return FText::FromString(TEXT("장비"));

		case EPRItemType::None:
		default:
			return FText::GetEmpty();
		}
	}

	void AddDetailLine(FPRItemTooltipViewData& ViewData, const FText& Label, const FText& Value)
	{
		FPRItemTooltipDetailLineViewData DetailLine;
		DetailLine.Label = Label;
		DetailLine.Value = Value;
		ViewData.DetailLines.Add(DetailLine);
	}

	FText BuildWeaponDisplayName(const UPRWeaponDataAsset* WeaponData, const UPRItemInstance_Weapon* WeaponItem)
	{
		if (!IsValid(WeaponData))
		{
			return FText::GetEmpty();
		}

		if (!IsValid(WeaponItem))
		{
			return WeaponData->GetDisplayName();
		}

		return FText::Format(
			FText::FromString(TEXT("{0}(+{1})")),
			WeaponData->GetDisplayName(),
			FText::AsNumber(WeaponItem->GetUpgradeLevel()));
	}

	int32 ResolveOwnedStackCount(const FPRInventoryItemSlotViewData& SlotViewData)
	{
		if (IsValid(SlotViewData.ItemInstance))
		{
			return SlotViewData.ItemInstance->GetStackCount();
		}

		return SlotViewData.bShowStackCount ? SlotViewData.StackCount : 0;
	}
}

FPRItemTooltipViewData UPRItemTooltipViewDataBuilder::BuildTooltipViewData(const FPRInventoryItemSlotViewData& SlotViewData)
{
	FPRItemTooltipViewData ViewData;

	UPRItemDataAsset* ItemData = SlotViewData.ItemData.Get();
	if (!IsValid(ItemData))
	{
		return ViewData;
	}

	ViewData.bHasItem = true;
	ViewData.DisplayName = ItemData->GetDisplayName();
	ViewData.ItemTypeText = GetItemTypeText(ItemData->GetItemType());
	ViewData.Description = ItemData->GetDescription();

	if (const UPRWeaponDataAsset* WeaponData = Cast<UPRWeaponDataAsset>(ItemData))
	{
		const UPRItemInstance_Weapon* WeaponItem = Cast<UPRItemInstance_Weapon>(SlotViewData.ItemInstance);
		ViewData.DisplayName = BuildWeaponDisplayName(WeaponData, WeaponItem);
		AddDetailLine(ViewData, FText::FromString(TEXT("공격력")), UPRUserInterfaceStatics::ConvertFloatToText(WeaponData->BaseDamage));
		AddDetailLine(ViewData, FText::FromString(TEXT("탄창")), UPRUserInterfaceStatics::ConvertFloatToText(WeaponData->MaxMagazineAmmo));
		AddDetailLine(ViewData, FText::FromString(TEXT("예비탄창")), UPRUserInterfaceStatics::ConvertFloatToText(WeaponData->ReserveAmmoMultiplier, 1));
		AddDetailLine(
			ViewData,
			FText::FromString(TEXT("약점 배율")),
			FText::Format(FText::FromString(TEXT("{0}%")), UPRUserInterfaceStatics::ConvertFloatToText(WeaponData->WeakpointMultiplier * 100.0f)));
		return ViewData;
	}

	if (const UPREquipmentDataAsset* EquipmentData = Cast<UPREquipmentDataAsset>(ItemData))
	{
		AddDetailLine(
			ViewData,
			FText::FromString(TEXT("장비 슬롯")),
			UPRInventoryItemSlotViewDataBuilder::GetEquipmentSlotDisplayName(EquipmentData->GetSlotType()));
		return ViewData;
	}

	if (Cast<UPRConsumableDataAsset>(ItemData) || Cast<UPRMaterialDataAsset>(ItemData))
	{
		AddDetailLine(ViewData, FText::FromString(TEXT("보유 수량")), FText::AsNumber(ResolveOwnedStackCount(SlotViewData)));
		AddDetailLine(ViewData, FText::FromString(TEXT("최대 스택")), FText::AsNumber(ItemData->MaxStackCount));
	}

	return ViewData;
}

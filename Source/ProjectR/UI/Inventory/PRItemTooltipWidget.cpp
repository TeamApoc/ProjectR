// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRItemTooltipWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "ProjectR/ItemSystem/Data/PRItemDataAsset.h"

void UPRItemTooltipWidget::SetTooltipViewData(const FPRInventoryItemSlotViewData& InViewData)
{
	ViewData = InViewData;
	RefreshNativeDisplay();
}

FPRInventoryItemSlotViewData UPRItemTooltipWidget::GetTooltipViewData() const
{
	return ViewData;
}

void UPRItemTooltipWidget::RefreshNativeDisplay()
{
	if (IsValid(ItemNameText))
	{
		ItemNameText->SetText(ViewData.DisplayName);
	}

	if (IsValid(ItemTypeText))
	{
		ItemTypeText->SetText(GetItemTypeText());
	}

	if (IsValid(ItemIconImage))
	{
		ItemIconImage->SetBrushFromTexture(ViewData.Icon.Get());
	}

	if (IsValid(DescriptionText))
	{
		FText Description = FText::GetEmpty();
		if (ViewData.InventoryAction == EPRInventoryAction::Deactivate && !IsValid(ViewData.ItemData))
		{
			Description = FText::FromString(TEXT("현재 장착을 해제합니다."));
		}
		else if (ViewData.InventoryAction == EPRInventoryAction::Deactivate)
		{
			Description = FText::FromString(TEXT("현재 장착 중입니다."));
		}
		else if (ViewData.bSelected)
		{
			Description = FText::FromString(TEXT("현재 선택 중입니다."));
		}

		DescriptionText->SetText(Description);
	}
}

FText UPRItemTooltipWidget::GetItemTypeText() const
{
	switch (ViewData.ItemType)
	{
	case EPRItemType::Weapon:
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

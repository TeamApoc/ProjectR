// Copyright ProjectR. All Rights Reserved.

#include "PRShopItemDetailWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "ProjectR/Inventory/Data/PRItemDataAsset.h"

void UPRShopItemDetailWidget::SetSelectedItemDetails(const FPRShopItemSlotViewData& InViewData, EPRShopTabType InCurrentTabType, int32 InOwnedStackCount)
{
	ViewData = InViewData;
	CurrentTabType = InCurrentTabType;
	OwnedStackCount = InOwnedStackCount;
	RefreshNativeDisplay();
}

/*~ UUserWidget Interface ~*/

void UPRShopItemDetailWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	RefreshNativeDisplay();
}

void UPRShopItemDetailWidget::RefreshNativeDisplay()
{
	const FPRInventoryItemSlotViewData& ItemViewData = ViewData.ItemViewData;
	UPRItemDataAsset* ItemData = ItemViewData.ItemData.Get();
	const bool bHasSelectedItem = IsValid(ItemData);

	if (IsValid(SelectedItemNameText))
	{
		SelectedItemNameText->SetText(bHasSelectedItem ? ItemViewData.DisplayName : FText::FromString(TEXT("아이템 선택")));
	}

	if (IsValid(SelectedItemDescriptionText))
	{
		SelectedItemDescriptionText->SetText(bHasSelectedItem ? ItemData->GetDescription() : FText::GetEmpty());
	}

	if (IsValid(SelectedItemIconImage))
	{
		UTexture2D* ItemIcon = bHasSelectedItem ? ItemViewData.Icon.Get() : nullptr;
		if (IsValid(ItemIcon))
		{
			SelectedItemIconImage->SetVisibility(ESlateVisibility::Visible);
			SelectedItemIconImage->SetBrushFromTexture(ItemIcon);
		}
		else
		{
			SelectedItemIconImage->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	if (IsValid(SelectedItemPriceText))
	{
		SelectedItemPriceText->SetText(bHasSelectedItem ? FText::AsNumber(ViewData.UnitScrapPrice) : FText::GetEmpty());
	}

	if (IsValid(SelectedItemStockText))
	{
		SelectedItemStockText->SetText(MakeStockText(bHasSelectedItem));
	}

	if (IsValid(SelectedItemOwnedCountText))
	{
		SelectedItemOwnedCountText->SetText(bHasSelectedItem ? FText::AsNumber(OwnedStackCount) : FText::GetEmpty());
	}
}

FText UPRShopItemDetailWidget::MakeStockText(bool bHasSelectedItem) const
{
	if (!bHasSelectedItem || CurrentTabType == EPRShopTabType::Sell)
	{
		return FText::GetEmpty();
	}

	if (ViewData.bUnlimitedStock)
	{
		return FText::FromString(TEXT("제한 없음"));
	}

	return FText::AsNumber(ViewData.RemainingStock);
}

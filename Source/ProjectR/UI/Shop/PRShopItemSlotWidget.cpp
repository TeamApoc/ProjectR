// Copyright ProjectR. All Rights Reserved.

#include "PRShopItemSlotWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "InputCoreTypes.h"
#include "ProjectR/ItemSystem/Data/PRItemDataAsset.h"
#include "ProjectR/Player/PRPlayerController.h"
#include "ProjectR/UI/Components/PRUIControllerComponent.h"

void UPRShopItemSlotWidget::SetSlotViewData(const FPRShopItemSlotViewData& InViewData)
{
	ViewData = InViewData;
	RefreshNativeDisplay();
	if (bIsMouseHovering)
	{
		ShowTooltip();
	}
}

/*~ UUserWidget Interface ~*/

void UPRShopItemSlotWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (IsValid(ClickButton))
	{
		ClickButton->OnClicked.RemoveDynamic(this, &UPRShopItemSlotWidget::HandleClickButtonClicked);
		ClickButton->OnClicked.AddDynamic(this, &UPRShopItemSlotWidget::HandleClickButtonClicked);
	}

	RefreshNativeDisplay();
}

FReply UPRShopItemSlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && !IsValid(ClickButton))
	{
		OnClicked.Broadcast(ViewData);
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UPRShopItemSlotWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	bIsMouseHovering = true;
	ShowTooltip();
}

void UPRShopItemSlotWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	bIsMouseHovering = false;
	HideTooltip();

	Super::NativeOnMouseLeave(InMouseEvent);
}

void UPRShopItemSlotWidget::NativeDestruct()
{
	HideTooltip();
	Super::NativeDestruct();
}

void UPRShopItemSlotWidget::HandleClickButtonClicked()
{
	OnClicked.Broadcast(ViewData);
}

void UPRShopItemSlotWidget::RefreshNativeDisplay()
{
	const FPRInventoryItemSlotViewData& ItemViewData = ViewData.ItemViewData;

	if (IsValid(ItemNameText))
	{
		ItemNameText->SetText(ItemViewData.DisplayName);
	}

	if (IsValid(ItemIconImage))
	{
		UTexture2D* ItemIcon = ItemViewData.Icon.Get();
		if (IsValid(ItemIcon))
		{
			ItemIconImage->SetVisibility(ESlateVisibility::Visible);
			ItemIconImage->SetBrushFromTexture(ItemIcon);
		}
		else
		{
			ItemIconImage->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	if (IsValid(PriceText))
	{
		PriceText->SetText(FText::AsNumber(ViewData.UnitScrapPrice));
	}

	if (IsValid(StockText))
	{
		StockText->SetText(MakeStockText());
	}

	if (IsValid(OwnedCountText))
	{
		const bool bShowOwnedCount = ItemViewData.bShowStackCount && IsValid(ItemViewData.ItemData.Get());
		OwnedCountText->SetVisibility(bShowOwnedCount ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
		OwnedCountText->SetText(FText::AsNumber(ItemViewData.StackCount));
	}

	if (IsValid(SelectedIndicatorImage))
	{
		SelectedIndicatorImage->SetVisibility(ViewData.bSelected ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
}

FText UPRShopItemSlotWidget::MakeStockText() const
{
	if (ViewData.TabType == EPRShopTabType::Sell)
	{
		return FText::GetEmpty();
	}

	if (ViewData.bUnlimitedStock)
	{
		return FText::FromString(TEXT(""));
	}

	return FText::AsNumber(ViewData.RemainingStock);
}

void UPRShopItemSlotWidget::ShowTooltip()
{
	if (!IsValid(ViewData.ItemViewData.ItemData.Get()))
	{
		HideTooltip();
		return;
	}

	UPRUIControllerComponent* UIController = ResolveUIController();
	if (!IsValid(UIController))
	{
		return;
	}

	UIController->ShowItemTooltip(this, ViewData.ItemViewData);
}

void UPRShopItemSlotWidget::HideTooltip()
{
	if (UPRUIControllerComponent* UIController = ResolveUIController())
	{
		UIController->HideItemTooltip();
	}
}

UPRUIControllerComponent* UPRShopItemSlotWidget::ResolveUIController() const
{
	const APRPlayerController* PRPlayerController = Cast<APRPlayerController>(GetOwningPlayer());
	return IsValid(PRPlayerController) ? PRPlayerController->GetUIController() : nullptr;
}

// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRItemSlotWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "InputCoreTypes.h"
#include "ProjectR/Inventory/Data/PRItemDataAsset.h"
#include "ProjectR/UI/Inventory/PRItemTooltipWidget.h"

void UPRItemSlotWidget::SetSlotViewData(const FPRInventoryItemSlotViewData& InViewData)
{
	ViewData = InViewData;
	// 현재 표시 데이터를 네이티브 바인딩 위젯에 반영한다
	RefreshNativeDisplay();
	// 현재 표시 데이터로 툴팁 위젯을 갱신한다
	RefreshTooltipWidget();
}

/*~ UUserWidget Interface ~*/

void UPRItemSlotWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	// 버튼 이벤트를 바인딩한다
	if (IsValid(ClickButton))
	{
		ClickButton->OnClicked.RemoveDynamic(this, &UPRItemSlotWidget::HandleClickButtonClicked);
		ClickButton->OnClicked.AddDynamic(this, &UPRItemSlotWidget::HandleClickButtonClicked);
	}
	// 현재 표시 데이터를 네이티브 바인딩 위젯에 반영한다
	RefreshNativeDisplay();
	// 현재 표시 데이터로 툴팁 위젯을 갱신한다
	RefreshTooltipWidget();
}

FReply UPRItemSlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		OnRightClicked.Broadcast(ViewData);
		return FReply::Handled();
	}

	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && !IsValid(ClickButton))
	{
		OnLeftClicked.Broadcast(ViewData);
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UPRItemSlotWidget::HandleClickButtonClicked()
{
	OnLeftClicked.Broadcast(ViewData);
}

void UPRItemSlotWidget::RefreshNativeDisplay()
{
	if (IsValid(ItemNameText))
	{
		ItemNameText->SetText(ViewData.DisplayName);
	}

	if (IsValid(ItemIconImage))
	{
		ItemIconImage->SetBrushFromTexture(ViewData.Icon.Get());
	}
}

void UPRItemSlotWidget::RefreshTooltipWidget()
{
	if (!IsValid(TooltipWidgetClass) || !IsValid(ViewData.ItemData.Get()))
	{
		SetToolTip(nullptr);
		return;
	}

	APlayerController* OwningPlayer = GetOwningPlayer();
	if (!IsValid(OwningPlayer))
	{
		SetToolTip(nullptr);
		return;
	}

	UPRItemTooltipWidget* TooltipWidget = CreateWidget<UPRItemTooltipWidget>(OwningPlayer, TooltipWidgetClass);
	if (!IsValid(TooltipWidget))
	{
		SetToolTip(nullptr);
		return;
	}

	TooltipWidget->SetTooltipViewData(ViewData);
	SetToolTip(TooltipWidget);
}

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
		UTexture2D* ItemIcon = ViewData.Icon.Get();
		if (!IsValid(ItemIcon))
		{
			// 아이템 아이콘이 없으면 아이템 아이콘 이미지 숨김
			ItemIconImage->SetVisibility(ESlateVisibility::Hidden);
		}
		else
		{
			ItemIconImage->SetVisibility(ESlateVisibility::Visible);
			ItemIconImage->SetBrushFromTexture(ItemIcon);
		}
	}

	if (IsValid(EquippedIndicatorImage))
	{
		// 아이템 장착중 여부에 따라 이미지 숨김/표시
		const ESlateVisibility EquippedIndicatorVisibility = ViewData.bEquipped
			? ESlateVisibility::Visible
			: ESlateVisibility::Hidden;
		EquippedIndicatorImage->SetVisibility(EquippedIndicatorVisibility);
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

// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRItemSlotWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "InputCoreTypes.h"
#include "ProjectR/ItemSystem/Data/PRItemDataAsset.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance.h"
#include "ProjectR/UI/Inventory/PRItemTooltipWidget.h"
#include "ProjectR/UI/Inventory/PRItemTooltipViewDataBuilder.h"

void UPRItemSlotWidget::SetSlotViewData(const FPRInventoryItemSlotViewData& InViewData)
{
	ViewData = InViewData;
	// 현재 표시 데이터를 네이티브 바인딩 위젯에 반영한다
	RefreshNativeDisplay();
	if (IsValid(ActiveTooltipWidget))
	{
		// 현재 표시 데이터로 툴팁 위젯을 갱신한다
		RefreshTooltipWidget();
	}
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
	
	if (IsValid(EquippedIndicatorImage))
	{
		EquippedIndicatorImage->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UPRItemSlotWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	RefreshTooltipWidget();
}

void UPRItemSlotWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	ClearTooltipWidget();

	Super::NativeOnMouseLeave(InMouseEvent);
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
		// 실제 아이템 항목의 장착 상태 표시와 명령 항목 표시 분리
		const bool bShowEquippedIndicator = ViewData.InventoryAction == EPRInventoryAction::Deactivate
			&& IsValid(ViewData.ItemInstance)
			&& IsValid(ViewData.ItemData);
		const bool bShowIndicator = bShowEquippedIndicator || ViewData.bSelected;
		const ESlateVisibility EquippedIndicatorVisibility = bShowIndicator
			? ESlateVisibility::Visible
			: ESlateVisibility::Hidden;
		EquippedIndicatorImage->SetVisibility(EquippedIndicatorVisibility);
	}

	if (IsValid(StackCountText))
	{
		const bool bShowStackCount = ViewData.bShowStackCount && IsValid(ViewData.ItemData.Get());
		StackCountText->SetVisibility(bShowStackCount ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
		StackCountText->SetText(FText::AsNumber(ViewData.StackCount));
	}
}

void UPRItemSlotWidget::RefreshTooltipWidget()
{
	if (!IsValid(TooltipWidgetClass) || !IsValid(ViewData.ItemData.Get()))
	{
		ClearTooltipWidget();
		return;
	}

	APlayerController* OwningPlayer = GetOwningPlayer();
	if (!IsValid(OwningPlayer))
	{
		ClearTooltipWidget();
		return;
	}

	if (!IsValid(ActiveTooltipWidget))
	{
		ActiveTooltipWidget = CreateWidget<UPRItemTooltipWidget>(OwningPlayer, TooltipWidgetClass);
		if (!IsValid(ActiveTooltipWidget))
		{
			ClearTooltipWidget();
			return;
		}

		SetToolTip(ActiveTooltipWidget);
	}

	ActiveTooltipWidget->SetTooltipViewData(UPRItemTooltipViewDataBuilder::BuildTooltipViewData(ViewData));
}

void UPRItemSlotWidget::ClearTooltipWidget()
{
	SetToolTip(nullptr);
	ActiveTooltipWidget = nullptr;
}

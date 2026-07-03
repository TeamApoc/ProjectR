// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRPlayerMenuTabListWidget.h"

#include "Blueprint/WidgetTree.h"
#include "CommonActivatableWidgetSwitcher.h"
#include "CommonButtonBase.h"
#include "Components/HorizontalBox.h"
#include "Components/Widget.h"
#include "ProjectR/UI/TextButton/PRTextButton.h"

bool UPRPlayerMenuTabListWidget::RefreshTabs(UCommonActivatableWidgetSwitcher* InWidgetSwitcher, TSubclassOf<UCommonButtonBase> ButtonWidgetType, int32 DesiredTabIndex)
{
	if (!IsValid(TabButtonContainer))
	{
		return false;
	}

	if (IsDesignTime())
	{
		// 디자인 탭 정리
		TabButtonContainer->ClearChildren();
		if (!IsValid(InWidgetSwitcher) || !IsValid(ButtonWidgetType.Get()) || !IsValid(WidgetTree))
		{
			return false;
		}

		for (int32 ChildIndex = 0; ChildIndex < InWidgetSwitcher->GetChildrenCount(); ++ChildIndex)
		{
			UWidget* ChildWidget = InWidgetSwitcher->GetChildAt(ChildIndex);
			if (!IsValid(ChildWidget))
			{
				continue;
			}

			UCommonButtonBase* PreviewTabButton = WidgetTree->ConstructWidget<UCommonButtonBase>(ButtonWidgetType);
			if (!IsValid(PreviewTabButton))
			{
				continue;
			}

			// 디자인 탭 표시
			AddTabButtonToContainer(ChildWidget->GetFName(), PreviewTabButton);
		}

		return true;
	}

	if (!IsValid(InWidgetSwitcher) || !IsValid(ButtonWidgetType.Get()))
	{
		return false;
	}

	// 기존 탭 정리
	RemoveAllTabs();
	// 탭 컨테이너 정리
	TabButtonContainer->ClearChildren();

	// 스위처 연결
	SetLinkedSwitcher(InWidgetSwitcher);

	FName FallbackTabName = NAME_None;
	FName DesiredTabName = NAME_None;
	for (int32 ChildIndex = 0; ChildIndex < InWidgetSwitcher->GetChildrenCount(); ++ChildIndex)
	{
		UWidget* ChildWidget = InWidgetSwitcher->GetChildAt(ChildIndex);
		if (!IsValid(ChildWidget))
		{
			continue;
		}

		const FName ChildName = ChildWidget->GetFName();
		if (FallbackTabName.IsNone())
		{
			FallbackTabName = ChildName;
		}

		if (ChildIndex == DesiredTabIndex)
		{
			DesiredTabName = ChildName;
		}

		// 스위처 자식 기반 탭 등록
		RegisterTab(ChildName, ButtonWidgetType, ChildWidget, ChildIndex);
	}

	const FName SelectTabName = DesiredTabName.IsNone() ? FallbackTabName : DesiredTabName;
	if (!SelectTabName.IsNone())
	{
		// 요청 탭 선택
		SelectTabByID(SelectTabName, true);
	}

	// 탭 입력 수신
	SetListeningForInput(true);

	return true;
}

void UPRPlayerMenuTabListWidget::HandleTabCreation_Implementation(FName TabNameID, UCommonButtonBase* TabButton)
{
	Super::HandleTabCreation_Implementation(TabNameID, TabButton);

	AddTabButtonToContainer(TabNameID, TabButton);
}

void UPRPlayerMenuTabListWidget::HandleTabRemoval_Implementation(FName TabNameID, UCommonButtonBase* TabButton)
{
	Super::HandleTabRemoval_Implementation(TabNameID, TabButton);

	if (!IsValid(TabButtonContainer))
	{
		return;
	}

	TabButtonContainer->RemoveChild(TabButton);
}

void UPRPlayerMenuTabListWidget::AddTabButtonToContainer(FName TabNameID, UCommonButtonBase* TabButton)
{
	if (!IsValid(TabButtonContainer) || !IsValid(TabButton))
	{
		return;
	}

	if (UPRTextButton* TabTextButton = Cast<UPRTextButton>(TabButton))
	{
		// 탭 텍스트
		TabTextButton->SetText(FText::FromName(TabNameID));
	}

	TabButtonContainer->AddChild(TabButton);
}

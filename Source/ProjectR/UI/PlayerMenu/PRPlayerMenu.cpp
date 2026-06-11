// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRPlayerMenu.h"

#include "CommonActivatableWidgetSwitcher.h"
#include "CommonButtonBase.h"
#include "Components/Widget.h"
#include "PRPlayerMenuTabListWidget.h"

UPRPlayerMenu::UPRPlayerMenu()
{
	Layer = EPRUILayer::Menu;
	InputMode = EPBUIInputMode::UIOnly;
	bShowMouseCursor = true;
}

void UPRPlayerMenu::NativePreConstruct()
{
	// 부모 미리보기 처리
	Super::NativePreConstruct();

	if (!IsDesignTime() || !IsValid(TabList))
	{
		return;
	}

	TArray<FName> PreviewTabNames;
	if (IsValid(WidgetSwitcher))
	{
		for (int32 ChildIndex = 0; ChildIndex < WidgetSwitcher->GetChildrenCount(); ++ChildIndex)
		{
			UWidget* ChildWidget = WidgetSwitcher->GetChildAt(ChildIndex);
			if (!IsValid(ChildWidget))
			{
				continue;
			}

			// 디자인 탭 이름
			PreviewTabNames.Add(ChildWidget->GetFName());
		}
	}

	TabList->RebuildDesignPreviewTabs(PreviewTabNames, TabButtonClass);
}

void UPRPlayerMenu::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	RegisterSwitcherTabs();
}

EPRUIInputAction UPRPlayerMenu::GetUIInputAction(const FKey& Key) const
{
	if (Key == EKeys::Tab)
	{
		// 메뉴 입력 재사용 닫기
		return EPRUIInputAction::Cancel;
	}

	return Super::GetUIInputAction(Key);
}

void UPRPlayerMenu::RegisterSwitcherTabs()
{
	if (!IsValid(WidgetSwitcher))
	{
		return;
	}

	if (!IsValid(TabList))
	{
		return;
	}

	TabList->RemoveAllTabs();
	TabList->SetLinkedSwitcher(WidgetSwitcher);

	if (!IsValid(TabButtonClass.Get()))
	{
		return;
	}

	FName FirstTabName = NAME_None;
	for (int32 ChildIndex = 0; ChildIndex < WidgetSwitcher->GetChildrenCount(); ++ChildIndex)
	{
		UWidget* ChildWidget = WidgetSwitcher->GetChildAt(ChildIndex);
		if (!IsValid(ChildWidget))
		{
			continue;
		}

		// 스위처 자식 이름 기반 탭 등록
		const FName ChildName = ChildWidget->GetFName();
		if (FirstTabName.IsNone())
		{
			FirstTabName = ChildName;
		}

		TabList->RegisterTab(ChildName, TabButtonClass, ChildWidget, ChildIndex);
	}

	if (!FirstTabName.IsNone())
	{
		// 최초 탭 선택
		TabList->SelectTabByID(FirstTabName, true);
	}

	// CommonUI 표준 탭 입력 등록
	TabList->SetListeningForInput(true);
}

// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRPlayerMenu.h"

#include "CommonActivatableWidgetSwitcher.h"
#include "CommonButtonBase.h"
#include "CommonTabListWidgetBase.h"
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

	if (UPRPlayerMenuTabListWidget* PlayerMenuTabList = Cast<UPRPlayerMenuTabListWidget>(TabList))
	{
		PlayerMenuTabList->RebuildDesignPreviewTabs(PreviewTabNames, TabButtonClass);
	}
}

bool UPRPlayerMenu::RefreshRuntimeTabs()
{
	if (!IsValid(WidgetSwitcher))
	{
		return false;
	}

	if (!IsValid(TabList))
	{
		return false;
	}

	if (UPRPlayerMenuTabListWidget* PlayerMenuTabList = Cast<UPRPlayerMenuTabListWidget>(TabList))
	{
		return PlayerMenuTabList->RegisterRuntimeTabs(WidgetSwitcher, TabButtonClass);
	}

	return false;
}

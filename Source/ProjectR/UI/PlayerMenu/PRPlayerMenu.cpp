// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRPlayerMenu.h"

#include "CommonActivatableWidgetSwitcher.h"
#include "CommonButtonBase.h"
#include "Components/Widget.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "ProjectR/UI/Components/PRUIControllerComponent.h"
#include "PRPlayerMenuTabListWidget.h"

UPRPlayerMenu::UPRPlayerMenu()
{
	SetIsFocusable(true);
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

FReply UPRPlayerMenu::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Tab
		|| Key == EKeys::Escape
		|| Key == EKeys::Virtual_Back
		|| Key == EKeys::Gamepad_FaceButton_Right)
	{
		if (APlayerController* OwningPlayerController = GetOwningPlayer())
		{
			if (UPRUIControllerComponent* UIController = OwningPlayerController->FindComponentByClass<UPRUIControllerComponent>())
			{
				// 메뉴 닫기 입력을 전용 표시 경로로 위임
				UIController->ClosePlayerMenu();
				return FReply::Handled();
			}
		}

		// 전용 컨트롤러 경로가 없을 때 최소 정리
		DeactivateWidget();
		RemoveFromParent();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
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

// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#include "PRWidgetBase.h"

#include "Engine/LocalPlayer.h"
#include "ProjectR/System/PREventManagerSubsystem.h"
#include "ProjectR/UI/PRUIManagerSubsystem.h"

UPRWidgetBase::UPRWidgetBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// UIOnly 모드에서 키 입력을 받을 수 있도록 기본 포커스 허용
	SetIsFocusable(true);
}

UPREventManagerSubsystem* UPRWidgetBase::GetEventManager() const
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return nullptr;
	}
	return World->GetSubsystem<UPREventManagerSubsystem>();
}

EPRUIInputAction UPRWidgetBase::GetUIInputAction(const FKey& Key) const
{
	if (Key == EKeys::Enter
		|| Key == EKeys::SpaceBar
		|| Key == EKeys::Virtual_Accept
		|| Key == EKeys::Gamepad_FaceButton_Bottom)
	{
		return EPRUIInputAction::Confirm;
	}

	if (Key == EKeys::Escape
		|| Key == EKeys::Virtual_Back
		|| Key == EKeys::Gamepad_FaceButton_Right)
	{
		return EPRUIInputAction::Cancel;
	}

	if (Key == EKeys::Up || Key == EKeys::Gamepad_DPad_Up)
	{
		return EPRUIInputAction::NavigateUp;
	}

	if (Key == EKeys::Down || Key == EKeys::Gamepad_DPad_Down)
	{
		return EPRUIInputAction::NavigateDown;
	}

	if (Key == EKeys::Left || Key == EKeys::Gamepad_DPad_Left)
	{
		return EPRUIInputAction::NavigateLeft;
	}

	if (Key == EKeys::Right || Key == EKeys::Gamepad_DPad_Right)
	{
		return EPRUIInputAction::NavigateRight;
	}

	if (Key == EKeys::Q || Key == EKeys::Gamepad_LeftShoulder)
	{
		return EPRUIInputAction::TabLeft;
	}

	if (Key == EKeys::E || Key == EKeys::Gamepad_RightShoulder)
	{
		return EPRUIInputAction::TabRight;
	}

	return EPRUIInputAction::None;
}

FReply UPRWidgetBase::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InputMode != EPBUIInputMode::UIOnly)
	{
		return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
	}

	const EPRUIInputAction UIInputAction = GetUIInputAction(InKeyEvent.GetKey());
	if (UIInputAction == EPRUIInputAction::Cancel)
	{
		if (ULocalPlayer* OwningLocalPlayer = GetOwningLocalPlayer())
		{
			if (UPRUIManagerSubsystem* UIManager = OwningLocalPlayer->GetSubsystem<UPRUIManagerSubsystem>())
			{
				// 현재 포커스 위젯이 아닌 UI 스택 최상위 위젯 닫기
				UIManager->PopUI();
				return FReply::Handled();
			}
		}
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

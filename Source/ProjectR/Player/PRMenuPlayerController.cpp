// Copyright ProjectR. All Rights Reserved.

#include "PRMenuPlayerController.h"

#include "Components/Widget.h"

void APRMenuPlayerController::ApplyMenuInputMode(UWidget* FocusWidget)
{
	if (!IsLocalController())
	{
		return;
	}

	// 메뉴 전용 UI 입력 모드 적용
	FInputModeUIOnly InputMode;
	if (IsValid(FocusWidget))
	{
		InputMode.SetWidgetToFocus(FocusWidget->TakeWidget());
	}

	SetInputMode(InputMode);
	SetShowMouseCursor(true);
}

void APRMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	ApplyMenuInputMode(nullptr);
}

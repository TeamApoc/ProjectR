// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRInteractionHUDWidget.h"

#include "Components/ProgressBar.h"

UPRInteractionHUDWidget::UPRInteractionHUDWidget()
{
	// HUD 레이어, 입력 모드 영향 없음
	Layer = EPRUILayer::HUD;
	InputMode = EPBUIInputMode::None;
	bShowMouseCursor = false;
}

void UPRInteractionHUDWidget::SetProgress(float InPercent)
{
	if (!IsValid(ProgressBar))
	{
		return;
	}
	ProgressBar->SetPercent(FMath::Clamp(InPercent, 0.f, 1.f));
}

void UPRInteractionHUDWidget::SetHUDVisible(bool bVisible)
{
	SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (!bVisible && IsValid(ProgressBar))
	{
		ProgressBar->SetPercent(0.f);
	}
}

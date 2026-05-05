// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#include "PRFloatingTextWidget.h"
#include "Components/TextBlock.h"

void UPRFloatingTextWidget::InitFloatingText(const FPRFloatingTextParams& Params)
{
	SetText(Params.Text);
	SetTextColor(Params.Color);
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UPRFloatingTextWidget::SetText(const FText& InText)
{
	if (IsValid(ValueText))
	{
		ValueText->SetText(InText);
	}
}

void UPRFloatingTextWidget::SetTextColor(FLinearColor InColor)
{
	if (IsValid(ValueText))
	{
		ValueText->SetColorAndOpacity(FSlateColor(InColor));
	}
}

void UPRFloatingTextWidget::OnPlay_Implementation()
{
}

void UPRFloatingTextWidget::FinishFloatingText()
{
	OnFloatingTextFinished.ExecuteIfBound(this);
}

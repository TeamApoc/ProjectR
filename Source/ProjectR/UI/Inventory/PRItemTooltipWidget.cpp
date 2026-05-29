// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRItemTooltipWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"

void UPRItemTooltipWidget::SetTooltipViewData(const FPRItemTooltipViewData& InViewData)
{
	ViewData = InViewData;
	RefreshNativeDisplay();
}

FPRItemTooltipViewData UPRItemTooltipWidget::GetTooltipViewData() const
{
	return ViewData;
}

void UPRItemTooltipWidget::RefreshNativeDisplay()
{
	if (IsValid(ItemNameText))
	{
		ItemNameText->SetText(ViewData.DisplayName);
	}

	if (IsValid(ItemTypeText))
	{
		ItemTypeText->SetText(ViewData.ItemTypeText);
	}

	if (IsValid(ItemIconImage))
	{
		ItemIconImage->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (IsValid(DescriptionText))
	{
		DescriptionText->SetText(ViewData.Description);
	}

	const bool bUseSplitDetailText = IsValid(DetailLabelText) || IsValid(DetailValueText);
	if (IsValid(DetailLabelText))
	{
		DetailLabelText->SetText(MakeDetailLabelText());
	}

	if (IsValid(DetailValueText))
	{
		DetailValueText->SetText(MakeDetailValueText());
	}

	if (IsValid(DetailText))
	{
		DetailText->SetVisibility(bUseSplitDetailText ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
		if (!bUseSplitDetailText)
		{
			DetailText->SetText(MakeDetailText());
		}
	}
}

FText UPRItemTooltipWidget::MakeDetailText() const
{
	if (ViewData.DetailLines.IsEmpty())
	{
		return FText::GetEmpty();
	}

	FString DetailString;
	for (int32 Index = 0; Index < ViewData.DetailLines.Num(); ++Index)
	{
		const FPRItemTooltipDetailLineViewData& DetailLine = ViewData.DetailLines[Index];
		if (Index > 0)
		{
			DetailString.Append(TEXT("\n"));
		}

		DetailString.Append(FString::Printf(TEXT("%s: %s"), *DetailLine.Label.ToString(), *DetailLine.Value.ToString()));
	}

	return FText::FromString(DetailString);
}

FText UPRItemTooltipWidget::MakeDetailLabelText() const
{
	if (ViewData.DetailLines.IsEmpty())
	{
		return FText::GetEmpty();
	}

	FString DetailString;
	for (int32 Index = 0; Index < ViewData.DetailLines.Num(); ++Index)
	{
		const FPRItemTooltipDetailLineViewData& DetailLine = ViewData.DetailLines[Index];
		if (Index > 0)
		{
			DetailString.Append(TEXT("\n"));
		}

		DetailString.Append(DetailLine.Label.ToString());
	}

	return FText::FromString(DetailString);
}

FText UPRItemTooltipWidget::MakeDetailValueText() const
{
	if (ViewData.DetailLines.IsEmpty())
	{
		return FText::GetEmpty();
	}

	FString DetailString;
	for (int32 Index = 0; Index < ViewData.DetailLines.Num(); ++Index)
	{
		const FPRItemTooltipDetailLineViewData& DetailLine = ViewData.DetailLines[Index];
		if (Index > 0)
		{
			DetailString.Append(TEXT("\n"));
		}

		DetailString.Append(DetailLine.Value.ToString());
	}

	return FText::FromString(DetailString);
}

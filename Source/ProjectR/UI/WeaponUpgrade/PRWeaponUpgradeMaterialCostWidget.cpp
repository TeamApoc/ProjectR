// Copyright ProjectR. All Rights Reserved.

#include "PRWeaponUpgradeMaterialCostWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

void UPRWeaponUpgradeMaterialCostWidget::SetMaterialCostViewData(const FPRWeaponUpgradeMaterialCostViewData& InViewData)
{
	ViewData = InViewData;
	RefreshNativeDisplay();
}

/*~ UUserWidget Interface ~*/

void UPRWeaponUpgradeMaterialCostWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	RefreshNativeDisplay();
}

void UPRWeaponUpgradeMaterialCostWidget::RefreshNativeDisplay()
{
	if (IsValid(MaterialIconImage))
	{
		UTexture2D* MaterialIcon = ViewData.Icon.Get();
		if (IsValid(MaterialIcon))
		{
			MaterialIconImage->SetVisibility(ESlateVisibility::Visible);
			MaterialIconImage->SetBrushFromTexture(MaterialIcon);
		}
		else
		{
			MaterialIconImage->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	if (IsValid(MaterialQuantityText))
	{
		const FString QuantityText = FString::Printf(TEXT("%d / %d"), ViewData.OwnedQuantity, ViewData.RequiredQuantity);
		MaterialQuantityText->SetText(FText::FromString(QuantityText));
		MaterialQuantityText->SetColorAndOpacity(FSlateColor(ViewData.bEnough ? EnoughQuantityColor : NotEnoughQuantityColor));
	}

	if (IsValid(MaterialNameText))
	{
		MaterialNameText->SetText(ViewData.DisplayName);
	}
}

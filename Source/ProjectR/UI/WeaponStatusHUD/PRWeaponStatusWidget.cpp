// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRWeaponStatusWidget.h"

#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "ProjectR/UI/PRUserInterfaceStatics.h"

void UPRWeaponStatusWidget::SetSlotType(EPRWeaponSlotType InSlotType)
{
	SlotType = InSlotType;
}

void UPRWeaponStatusWidget::SetWeaponStatus(const FPRWeaponStatusViewData& ViewData)
{
	SlotType = ViewData.SlotType;

	if (!ViewData.bHasWeapon)
	{
		ClearWeaponStatus();
		return;
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);
	SetWeaponIcon(ViewData.WeaponIcon);
	SetMagazineAmmoText(ViewData.MagazineAmmo);
	SetReserveAmmoText(ViewData.ReserveAmmo);

	if (ViewData.bHasMod)
	{
		SetModIcon(ViewData.ModIcon);
		SetModGaugePercent(ViewData.ModGaugePercent);
		SetModStackText(ViewData.ModStackCount);
	}
	else
	{
		ClearModStatus();
	}
}

void UPRWeaponStatusWidget::SetWeaponIcon(UTexture2D* Icon)
{
	if (!IsValid(WeaponIconImage))
	{
		return;
	}

	if (!IsValid(Icon))
	{
		WeaponIconImage->SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	WeaponIconImage->SetBrushFromTexture(Icon);
	WeaponIconImage->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UPRWeaponStatusWidget::SetMagazineAmmoText(float MagazineAmmo)
{
	if (!IsValid(MagazineAmmoText))
	{
		return;
	}

	MagazineAmmoText->SetText(UPRUserInterfaceStatics::ConvertFloatToText(MagazineAmmo));
	MagazineAmmoText->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UPRWeaponStatusWidget::SetReserveAmmoText(float ReserveAmmo)
{
	if (!IsValid(ReserveAmmoText))
	{
		return;
	}

	ReserveAmmoText->SetText(UPRUserInterfaceStatics::ConvertFloatToText(ReserveAmmo));
	ReserveAmmoText->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UPRWeaponStatusWidget::SetModIcon(UTexture2D* Icon)
{
	if (!IsValid(ModIconImage))
	{
		return;
	}

	if (!IsValid(Icon))
	{
		ModIconImage->SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	ModIconImage->SetBrushFromTexture(Icon);
	ModIconImage->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UPRWeaponStatusWidget::SetModGaugePercent(float Percent)
{
	if (!IsValid(ModGaugeBar))
	{
		return;
	}

	ModGaugeBar->SetPercent(FMath::Clamp(Percent, 0.0f, 1.0f));
	ModGaugeBar->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UPRWeaponStatusWidget::SetModStackText(float StackCount)
{
	if (!IsValid(ModStackText))
	{
		return;
	}

	ModStackText->SetText(UPRUserInterfaceStatics::ConvertFloatToText(StackCount));
	ModStackText->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UPRWeaponStatusWidget::ClearWeaponStatus()
{
	if (IsValid(WeaponIconImage))
	{
		WeaponIconImage->SetVisibility(ESlateVisibility::Hidden);
	}

	SetMagazineAmmoText(0.0f);
	SetReserveAmmoText(0.0f);

	ClearModStatus();
}

void UPRWeaponStatusWidget::ClearModStatus()
{
	if (IsValid(ModIconImage))
	{
		ModIconImage->SetVisibility(ESlateVisibility::Hidden);
	}

	if (IsValid(ModGaugeBar))
	{
		ModGaugeBar->SetPercent(0.0f);
		ModGaugeBar->SetVisibility(ESlateVisibility::Hidden);
	}

	if (IsValid(ModStackText))
	{
		ModStackText->SetText(UPRUserInterfaceStatics::ConvertFloatToText(0.0f));
		ModStackText->SetVisibility(ESlateVisibility::Hidden);
	}
}

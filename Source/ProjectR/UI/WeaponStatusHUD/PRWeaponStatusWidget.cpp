// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (Mod 충전 게이지 비주얼 연동 구현)
// Author: 이건주 (무기 종류 아이콘 실시간 갱신 구현)
#include "PRWeaponStatusWidget.h"

#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/System/PREventManagerSubsystem.h"
#include "ProjectR/UI/PRUserInterfaceStatics.h"

void UPRWeaponStatusWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	if (HighlightBorder)
	{
		HighlightBorder->SetVisibility(ESlateVisibility::Collapsed);
	}
}

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
	
	UnbindHighlightEvent();
	BindHighlightEvent();

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
	UnbindHighlightEvent();
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

void UPRWeaponStatusWidget::BindHighlightEvent()
{
	// 모드 활성화 수신 대기
	if (UPREventManagerSubsystem* EventManager = GetEventManager())
	{
		HighlightEventDelegateHandle = EventManager->Listen(PRGameplayTags::Event_Player_ModActivation,
			FPREventMulticast::FDelegate::CreateUObject(this, &ThisClass::HandleModActivation));
	}
}

void UPRWeaponStatusWidget::UnbindHighlightEvent()
{
	if (HighlightEventDelegateHandle.IsValid())
	{
		if (UPREventManagerSubsystem* EventManager = GetEventManager())
		{
			EventManager->UnlistenAll(HighlightEventDelegateHandle);
		}
	}
}

void UPRWeaponStatusWidget::HandleModActivation(FGameplayTag EventTag, const FInstancedStruct& Payload)
{
	const FPRModActivationPayload* EventData = Payload.GetPtr<FPRModActivationPayload>();
	if (!EventData)
	{
		return;
	}
	
	if (EventData->SlotType != SlotType)
	{
		return;
	}
	
	if (EventData->bActivated)
	{
		if (HighlightBorder)
		{
			HighlightBorder->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		if (HighlightAnimation)
		{
			PlayAnimation(HighlightAnimation,0.f,0);
		}
	}
	else
	{
		if (HighlightBorder)
		{
			HighlightBorder->SetVisibility(ESlateVisibility::Hidden);
		}
		if (HighlightAnimation)
		{
			StopAnimation(HighlightAnimation);
		}
	}
}

// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRHUDWidget.h"

#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/System/PREventManagerSubsystem.h"
#include "ProjectR/UI/Crosshair/PRCrosshairWidget.h"

UPRHUDWidget::UPRHUDWidget()
{
	// HUD 컨테이너는 HUD 레이어에 위치하며 입력에 영향을 주지 않는다
	Layer = EPRUILayer::HUD;
	InputMode = EPBUIInputMode::None;
	bShowMouseCursor = false;
}

void UPRHUDWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 크로스헤어가 BP 레이아웃에 포함된 경우에만 에임 이벤트를 구독한다
	if (!IsValid(CrosshairWidget))
	{
		return;
	}

	// 초기 상태는 숨김. Aim.Start 신호를 받기 전까지 보이지 않는다
	SetCrosshairVisible(false);

	if (UPREventManagerSubsystem* EventMgr = GetEventManager())
	{
		AimStartHandle = EventMgr->Listen(
			PRGameplayTags::Event_Player_Aim_Start,
			FPREventMulticast::FDelegate::CreateUObject(this, &UPRHUDWidget::HandleAimStart));

		AimEndHandle = EventMgr->Listen(
			PRGameplayTags::Event_Player_Aim_End,
			FPREventMulticast::FDelegate::CreateUObject(this, &UPRHUDWidget::HandleAimEnd));
	}
}

void UPRHUDWidget::NativeDestruct()
{
	UnbindFromEventManager();
	Super::NativeDestruct();
}

void UPRHUDWidget::HandleAimStart(FGameplayTag EventTag, const FInstancedStruct& Payload)
{
	SetCrosshairVisible(true);
}

void UPRHUDWidget::HandleAimEnd(FGameplayTag EventTag, const FInstancedStruct& Payload)
{
	SetCrosshairVisible(false);
}

void UPRHUDWidget::UnbindFromEventManager()
{
	if (UPREventManagerSubsystem* EventMgr = GetEventManager())
	{
		if (AimStartHandle.IsValid())
		{
			EventMgr->Unlisten(PRGameplayTags::Event_Player_Aim_Start, AimStartHandle);
		}
		if (AimEndHandle.IsValid())
		{
			EventMgr->Unlisten(PRGameplayTags::Event_Player_Aim_End, AimEndHandle);
		}
	}
	AimStartHandle.Reset();
	AimEndHandle.Reset();
}

void UPRHUDWidget::SetCrosshairVisible(bool bVisible)
{
	if (!IsValid(CrosshairWidget))
	{
		return;
	}
	CrosshairWidget->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

UPREventManagerSubsystem* UPRHUDWidget::GetEventManager() const
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return nullptr;
	}
	return World->GetSubsystem<UPREventManagerSubsystem>();
}

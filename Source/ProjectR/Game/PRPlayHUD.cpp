// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRPlayHUD.h"

#include "ProjectR/UI/HUD/PRHUDWidget.h"
#include "ProjectR/UI/PRUIManagerSubsystem.h"

APRPlayHUD::APRPlayHUD()
{
	PrimaryActorTick.bCanEverTick = false;
}

void APRPlayHUD::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PC = GetOwningPlayerController();
	if (!IsValid(PC) || !IsValid(HUDWidgetClass))
	{
		return;
	}

	HUDWidget = CreateWidget<UPRHUDWidget>(PC, HUDWidgetClass);
	if (!IsValid(HUDWidget))
	{
		return;
	}

	// UIManager를 통해 Push. 레이어/ZOrder 정책을 매니저가 일원 관리
	if (UPRUIManagerSubsystem* UIManager = GetUIManager())
	{
		UIManager->PushUIInstance(HUDWidget);
	}
	else
	{
		// UIManager가 없는 예외 케이스에 대한 폴백
		HUDWidget->AddToViewport();
	}
}

void APRPlayHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(HUDWidget))
	{
		if (UPRUIManagerSubsystem* UIManager = GetUIManager())
		{
			UIManager->PopUI(HUDWidget);
		}
		else
		{
			HUDWidget->RemoveFromParent();
		}
		HUDWidget = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

UPRUIManagerSubsystem* APRPlayHUD::GetUIManager() const
{
	APlayerController* PC = GetOwningPlayerController();
	if (!IsValid(PC))
	{
		return nullptr;
	}
	ULocalPlayer* LP = PC->GetLocalPlayer();
	if (!IsValid(LP))
	{
		return nullptr;
	}
	return LP->GetSubsystem<UPRUIManagerSubsystem>();
}

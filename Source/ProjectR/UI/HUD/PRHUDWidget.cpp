// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRHUDWidget.h"

#include "PRInteractionHUDWidget.h"
#include "ProjectR/Interaction/PRInteractionAction.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "ProjectR/System/PREventManagerSubsystem.h"
#include "ProjectR/UI/Crosshair/PRCrosshairWidget.h"
#include "ProjectR/UI/WeaponStatusHUD/PRWeaponHUDWidget.h"

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

	UPREventManagerSubsystem* EventMgr = GetEventManager();
	if (!IsValid(EventMgr))
	{
		return;
	}
	
	bool bPlayerReady = false;
	if (APlayerController* OwningPlayerController = GetOwningPlayer())
	{
		if (APRPlayerState* PS = OwningPlayerController->GetPlayerState<APRPlayerState>())
		{
			bPlayerReady = true; 
		}
	}
	
	if (bPlayerReady)
	{
		OnPlayerReady();
	}
	else
	{
		EventHandles.Add(EventMgr->Listen(
			PRGameplayTags::Player_Ready,
			FPREventMulticast::FDelegate::CreateUObject(this, &UPRHUDWidget::HandlePlayerReady)));
	}

	// 크로스헤어가 BP 레이아웃에 포함된 경우에만 에임 이벤트를 구독한다
	if (IsValid(CrosshairWidget))
	{
		// 초기 상태는 숨김. Aim.Start 신호를 받기 전까지 보이지 않는다
		SetCrosshairVisible(false);

		EventHandles.Add(EventMgr->Listen(
			PRGameplayTags::Event_Player_Aim_Start,
			FPREventMulticast::FDelegate::CreateUObject(this, &UPRHUDWidget::HandleAimStart)));

		EventHandles.Add(EventMgr->Listen(
			PRGameplayTags::Event_Player_Aim_End,
			FPREventMulticast::FDelegate::CreateUObject(this, &UPRHUDWidget::HandleAimEnd)));
	}

	// 상호작용 HUD 가 BP 레이아웃에 포함된 경우에만 Hold 이벤트를 구독한다
	if (IsValid(InteractionHUD))
	{
		InteractionHUD->SetHUDVisible(false);

		EventHandles.Add(EventMgr->Listen(
			PRGameplayTags::Event_Player_Interaction_Hold,
			FPREventMulticast::FDelegate::CreateUObject(this, &UPRHUDWidget::HandleInteractionHold)));
	}
}

void UPRHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UPRHUDWidget::NativeDestruct()
{
	UnbindFromEventManager();
	Super::NativeDestruct();
}

void UPRHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Hold 진행 중일 때 진행도 보간. HoldDuration 이 0 이하인 경우는 Start 즉시 완료로 간주
	if (bIsHoldActive && IsValid(InteractionHUD))
	{
		HoldElapsed += InDeltaTime;
		const float Percent = HoldDuration > KINDA_SMALL_NUMBER
			? FMath::Clamp(HoldElapsed / HoldDuration, 0.f, 1.f)
			: 1.f;
		InteractionHUD->SetProgress(Percent);
	}
}

void UPRHUDWidget::OnPlayerReady()
{
	if (WeaponHUD)
	{
		WeaponHUD->InitializeWeaponHUD();
	}
}

void UPRHUDWidget::HandlePlayerReady(FGameplayTag EventTag, const FInstancedStruct& Payload)
{
	OnPlayerReady();
}

void UPRHUDWidget::HandleAimStart(FGameplayTag EventTag, const FInstancedStruct& Payload)
{
	SetCrosshairVisible(true);
}

void UPRHUDWidget::HandleAimEnd(FGameplayTag EventTag, const FInstancedStruct& Payload)
{
	SetCrosshairVisible(false);
}

void UPRHUDWidget::HandleInteractionHold(FGameplayTag EventTag, const FInstancedStruct& Payload)
{
	if (!IsValid(InteractionHUD))
	{
		return;
	}

	const FPRInteractionHoldEventPayload* Data = Payload.GetPtr<FPRInteractionHoldEventPayload>();
	if (Data == nullptr)
	{
		return;
	}

	switch (Data->Phase)
	{
	case EPRInteractionHoldPhase::Start:
		HoldDuration = Data->HoldDuration;
		HoldElapsed = 0.f;
		bIsHoldActive = true;
		InteractionHUD->SetHUDVisible(true);
		InteractionHUD->SetProgress(0.f);
		break;

	case EPRInteractionHoldPhase::Canceled:
		bIsHoldActive = false;
		HoldElapsed = 0.f;
		HoldDuration = 0.f;
		InteractionHUD->SetHUDVisible(false);
		break;

	case EPRInteractionHoldPhase::Finished:
		bIsHoldActive = false;
		// 완료 펄스를 위해 한 프레임은 100% 로 채운 뒤 숨김 처리
		InteractionHUD->SetProgress(1.f);
		InteractionHUD->SetHUDVisible(false);
		HoldElapsed = 0.f;
		HoldDuration = 0.f;
		break;
	}
}

void UPRHUDWidget::UnbindFromEventManager()
{
	if (UPREventManagerSubsystem* EventMgr = GetEventManager())
	{
		for (auto& Handle : EventHandles)
		{
			EventMgr->UnlistenAll(Handle);
		}
	}
	EventHandles.Reset();
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

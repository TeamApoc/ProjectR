// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRCrosshairWidget.h"

#include "InstancedStruct.h"
#include "PRCrosshairConfig.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/System/PREventManagerSubsystem.h"

using namespace PRGameplayTags;

UPRCrosshairWidget::UPRCrosshairWidget()
{
	// 크로스헤어는 HUD 레이어, 입력 모드 영향 없음
	Layer = EPRUILayer::HUD;
	InputMode = EPBUIInputMode::None;
	bShowMouseCursor = false;
}

void UPRCrosshairWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (UPREventManagerSubsystem* EventMgr = GetEventManager())
	{
		HitShotHandle = EventMgr->Listen(
			Event_Player_HitShot,
			FPREventMulticast::FDelegate::CreateUObject(this, &UPRCrosshairWidget::HandleHitShot));

		RecoilHandle = EventMgr->Listen(
			Event_Player_Recoil,
			FPREventMulticast::FDelegate::CreateUObject(this, &UPRCrosshairWidget::HandleRecoil));

		ChangeCrosshairHandle = EventMgr->Listen(
			Event_Player_ChangeCrosshair,
			FPREventMulticast::FDelegate::CreateUObject(this, &UPRCrosshairWidget::HandleChangeCrosshair));
	}
}

void UPRCrosshairWidget::NativeDestruct()
{
	UnbindFromEventManager();
	Super::NativeDestruct();
}

void UPRCrosshairWidget::HandleHitShot(FGameplayTag EventTag, const FInstancedStruct& Payload)
{
	// 인터페이스 BP 구현 호출. 페이로드는 사용하지 않음
	Execute_OnHit(this);
}

void UPRCrosshairWidget::HandleRecoil(FGameplayTag EventTag, const FInstancedStruct& Payload)
{
	float Speed = 0.f;
	float Strength = 0.f;

	if (const FPRRecoilEventPayload* RecoilData = Payload.GetPtr<FPRRecoilEventPayload>())
	{
		Speed = RecoilData->Speed;
		Strength = RecoilData->Strength;
	}

	Execute_OnRecoil(this, Speed, Strength);
}

void UPRCrosshairWidget::HandleChangeCrosshair(FGameplayTag EventTag, const FInstancedStruct& Payload)
{
	const UPRCrosshairConfig* NewConfig = nullptr;
	if (const FPRChangeCrosshairEventPayload* Data = Payload.GetPtr<FPRChangeCrosshairEventPayload>())
	{
		NewConfig = Data->Config;
	}

	// nullptr 도 그대로 전달. BP 측에서 초기화/해제 분기 처리
	Execute_InitCrosshair(this, NewConfig);
}

void UPRCrosshairWidget::UnbindFromEventManager()
{
	if (UPREventManagerSubsystem* EventMgr = GetEventManager())
	{
		if (HitShotHandle.IsValid())
		{
			EventMgr->Unlisten(Event_Player_HitShot, HitShotHandle);
		}
		if (RecoilHandle.IsValid())
		{
			EventMgr->Unlisten(Event_Player_Recoil, RecoilHandle);
		}
		if (ChangeCrosshairHandle.IsValid())
		{
			EventMgr->Unlisten(Event_Player_ChangeCrosshair, ChangeCrosshairHandle);
		}
	}
	HitShotHandle.Reset();
	RecoilHandle.Reset();
	ChangeCrosshairHandle.Reset();
}

UPREventManagerSubsystem* UPRCrosshairWidget::GetEventManager() const
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return nullptr;
	}
	return World->GetSubsystem<UPREventManagerSubsystem>();
}

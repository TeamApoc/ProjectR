// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#include "PRWidgetBase.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Engine/LocalPlayer.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectR/System/PREventManagerSubsystem.h"
#include "ProjectR/System/PRDeveloperSettings.h"
#include "ProjectR/UI/PRUIManagerSubsystem.h"
#include "ProjectR/UI/PRUISoundDataAsset.h"
#include "Sound/SoundBase.h"

UPRWidgetBase::UPRWidgetBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// UIOnly 모드에서 키 입력을 받을 수 있도록 기본 포커스 허용
	SetIsFocusable(true);
}

UPREventManagerSubsystem* UPRWidgetBase::GetEventManager() const
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return nullptr;
	}
	return World->GetSubsystem<UPREventManagerSubsystem>();
}

EPRUIInputAction UPRWidgetBase::GetUIInputAction(const FKey& Key) const
{
	if (Key == EKeys::Enter
		|| Key == EKeys::SpaceBar
		|| Key == EKeys::Virtual_Accept
		|| Key == EKeys::Gamepad_FaceButton_Bottom)
	{
		return EPRUIInputAction::Confirm;
	}

	if (Key == EKeys::Escape
		|| Key == EKeys::Virtual_Back
		|| Key == EKeys::Gamepad_FaceButton_Right)
	{
		return EPRUIInputAction::Cancel;
	}

	if (Key == EKeys::Up || Key == EKeys::Gamepad_DPad_Up)
	{
		return EPRUIInputAction::NavigateUp;
	}

	if (Key == EKeys::Down || Key == EKeys::Gamepad_DPad_Down)
	{
		return EPRUIInputAction::NavigateDown;
	}

	if (Key == EKeys::Left || Key == EKeys::Gamepad_DPad_Left)
	{
		return EPRUIInputAction::NavigateLeft;
	}

	if (Key == EKeys::Right || Key == EKeys::Gamepad_DPad_Right)
	{
		return EPRUIInputAction::NavigateRight;
	}

	if (Key == EKeys::Q || Key == EKeys::Gamepad_LeftShoulder)
	{
		return EPRUIInputAction::TabLeft;
	}

	if (Key == EKeys::E || Key == EKeys::Gamepad_RightShoulder)
	{
		return EPRUIInputAction::TabRight;
	}

	return EPRUIInputAction::None;
}

void UPRWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();

	BindDefaultButtonSoundEvents();
}

void UPRWidgetBase::NativeDestruct()
{
	UnbindDefaultButtonSoundEvents();

	Super::NativeDestruct();
}

FReply UPRWidgetBase::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InputMode != EPBUIInputMode::UIOnly)
	{
		return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
	}

	const EPRUIInputAction UIInputAction = GetUIInputAction(InKeyEvent.GetKey());
	if (UIInputAction == EPRUIInputAction::Cancel)
	{
		if (ULocalPlayer* OwningLocalPlayer = GetOwningLocalPlayer())
		{
			if (UPRUIManagerSubsystem* UIManager = OwningLocalPlayer->GetSubsystem<UPRUIManagerSubsystem>())
			{
				// 현재 포커스 위젯이 아닌 UI 스택 최상위 위젯 닫기
				UIManager->PopUI();
				return FReply::Handled();
			}
		}
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UPRWidgetBase::BindDefaultButtonSoundEvents()
{
	UnbindDefaultButtonSoundEvents();

	if (!IsValid(WidgetTree))
	{
		return;
	}

	WidgetTree->ForEachWidget([this](UWidget* Widget)
	{
		UButton* Button = Cast<UButton>(Widget);
		if (!IsValid(Button))
		{
			return;
		}

		// 버튼 Style 사운드는 UMG 자체 재생 대상이므로 PRUISoundData 쿨타임 제외
		const bool bHasHoveredStyleSound = HasButtonHoveredStyleSound(Button);
		const bool bHasClickedStyleSound = HasButtonClickedStyleSound(Button);
		bool bBoundDefaultSound = false;

		if (!bHasHoveredStyleSound)
		{
			Button->OnHovered.AddDynamic(this, &ThisClass::HandleDefaultButtonHovered);
			bBoundDefaultSound = true;
		}

		if (!bHasClickedStyleSound)
		{
			Button->OnClicked.AddDynamic(this, &ThisClass::HandleDefaultButtonClicked);
			bBoundDefaultSound = true;
		}

		if (bBoundDefaultSound)
		{
			DefaultSoundBoundButtons.Add(Button);
		}
	});
}

void UPRWidgetBase::UnbindDefaultButtonSoundEvents()
{
	for (UButton* Button : DefaultSoundBoundButtons)
	{
		if (!IsValid(Button))
		{
			continue;
		}

		Button->OnHovered.RemoveDynamic(this, &ThisClass::HandleDefaultButtonHovered);
		Button->OnClicked.RemoveDynamic(this, &ThisClass::HandleDefaultButtonClicked);
	}

	DefaultSoundBoundButtons.Reset();
}

const UPRUISoundDataAsset* UPRWidgetBase::GetDefaultUISoundData() const
{
	const UPRDeveloperSettings* DeveloperSettings = GetDefault<UPRDeveloperSettings>();
	if (!IsValid(DeveloperSettings))
	{
		return nullptr;
	}

	return DeveloperSettings->GetDefaultUISoundDataSync();
}

bool UPRWidgetBase::CanPlayUISound(float LastPlayTime, float Cooldown) const
{
	if (Cooldown <= 0.0f)
	{
		return true;
	}

	const UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return false;
	}

	return World->GetRealTimeSeconds() - LastPlayTime >= Cooldown;
}

bool UPRWidgetBase::HasButtonHoveredStyleSound(const UButton* Button) const
{
	if (!IsValid(Button))
	{
		return false;
	}

	return IsValid(Cast<USoundBase>(Button->GetStyle().HoveredSlateSound.GetResourceObject()));
}

bool UPRWidgetBase::HasButtonClickedStyleSound(const UButton* Button) const
{
	if (!IsValid(Button))
	{
		return false;
	}

	return IsValid(Cast<USoundBase>(Button->GetStyle().ClickedSlateSound.GetResourceObject()));
}

void UPRWidgetBase::PlayUISound(USoundBase* Sound)
{
	if (!IsValid(Sound))
	{
		return;
	}

	UGameplayStatics::PlaySound2D(this, Sound, 1.0f, 1.0f, 0.0f, nullptr, nullptr, true);
}

void UPRWidgetBase::HandleDefaultButtonHovered()
{
	const UPRUISoundDataAsset* UISoundData = GetDefaultUISoundData();
	if (!IsValid(UISoundData) || !CanPlayUISound(LastHoveredSoundPlayTime, UISoundData->PlaybackCooldown))
	{
		return;
	}

	PlayUISound(UISoundData->HoveredSound);
	if (const UWorld* World = GetWorld())
	{
		LastHoveredSoundPlayTime = World->GetRealTimeSeconds();
	}
}

void UPRWidgetBase::HandleDefaultButtonClicked()
{
	const UPRUISoundDataAsset* UISoundData = GetDefaultUISoundData();
	if (!IsValid(UISoundData) || !CanPlayUISound(LastClickedSoundPlayTime, UISoundData->PlaybackCooldown))
	{
		return;
	}

	PlayUISound(UISoundData->ClickedSound);
	if (const UWorld* World = GetWorld())
	{
		LastClickedSoundPlayTime = World->GetRealTimeSeconds();
	}
}

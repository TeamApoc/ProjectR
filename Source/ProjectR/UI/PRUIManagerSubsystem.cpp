// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#include "PRUIManagerSubsystem.h"
#include "GameFramework/PlayerController.h"

void UPRUIManagerSubsystem::Deinitialize()
{
	ResetSystem();
	Super::Deinitialize();
}

void UPRUIManagerSubsystem::ResetSystem()
{
	for (int32 Index = UIStack.Num() - 1; Index >= 0; --Index)
	{
		UPRWidgetBase* Widget = UIStack[Index];
		if (!IsValid(Widget))
		{
			continue;
		}
		
		Widget->RemoveFromParent();
	}
	UIStack.Reset();
	
	if (APlayerController* PC = GetLocalPlayer() ? GetLocalPlayer()->GetPlayerController(GetWorld()) : nullptr)
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(InputMode);
	}
}

UPRWidgetBase* UPRUIManagerSubsystem::PushUI(TSubclassOf<UPRWidgetBase> WidgetClass)
{
	if (!WidgetClass)
	{
		return nullptr;
	}

	APlayerController* PC = GetLocalPlayer() ? GetLocalPlayer()->GetPlayerController(GetWorld()) : nullptr;
	if (!PC)
	{
		return nullptr;
	}

	UPRWidgetBase* Widget = CreateWidget<UPRWidgetBase>(PC, WidgetClass);
	if (!Widget)
	{
		return nullptr;
	}

	Widget->AddToViewport(GetZOrderForLayer(Widget->Layer));
	UIStack.Insert(Widget, FindLayerInsertIndex(Widget->Layer));
	UpdateInputMode();
	return Widget;
}

UPRWidgetBase* UPRUIManagerSubsystem::PushUIInstance(UPRWidgetBase* Instance)
{
	if (!IsValid(Instance))
	{
		return nullptr;
	}

	// 이미 스택에 존재하면 중복 추가 방지
	if (UIStack.Contains(Instance))
	{
		return Instance;
	}

	// 외부에서 이미 뷰포트에 추가하지 않은 경우에만 부착
	if (!Instance->IsInViewport())
	{
		Instance->AddToViewport(GetZOrderForLayer(Instance->Layer));
	}

	UIStack.Insert(Instance, FindLayerInsertIndex(Instance->Layer));
	UpdateInputMode();
	return Instance;
}

UPRWidgetBase* UPRUIManagerSubsystem::PopUI(UPRWidgetBase* Instance)
{
	CleanupInvalidEntries();

	if (UIStack.IsEmpty())
	{
		return nullptr;
	}

	UPRWidgetBase* Target = Instance;
	if (!Target)
	{
		Target = UIStack.Last();
	}

	if (Target)
	{
		UIStack.Remove(Target);
		Target->RemoveFromParent();
	}

	UpdateInputMode();
	
	return Target;
}

bool UPRUIManagerSubsystem::IsUIActive(TSubclassOf<UPRWidgetBase> WidgetClass) const
{
	for (const TObjectPtr<UPRWidgetBase>& Widget : UIStack)
	{
		if (IsValid(Widget) && Widget->IsA(WidgetClass))
		{
			return true;
		}
	}
	return false;
}

UPRWidgetBase* UPRUIManagerSubsystem::GetTopWidget() const
{
	for (int32 i = UIStack.Num() - 1; i >= 0; --i)
	{
		if (IsValid(UIStack[i]))
		{
			return UIStack[i];
		}
	}
	return nullptr;
}

void UPRUIManagerSubsystem::UpdateInputMode()
{
	CleanupInvalidEntries();

	APlayerController* PC = GetLocalPlayer() ? GetLocalPlayer()->GetPlayerController(GetWorld()) : nullptr;
	if (!PC)
	{
		return;
	}

	// 스택 위에서부터 InputMode != None인 첫 엔트리 탐색
	EPBUIInputMode EffectiveMode = EPBUIInputMode::None;
	bool bEffectiveCursor = false;
	for (int32 i = UIStack.Num() - 1; i >= 0; --i)
	{
		if (IsValid(UIStack[i]) && UIStack[i]->InputMode != EPBUIInputMode::None)
		{
			EffectiveMode = UIStack[i]->InputMode;
			bEffectiveCursor = UIStack[i]->bShowMouseCursor;
			break;
		}
	}

	switch (EffectiveMode)
	{
	case EPBUIInputMode::UIOnly:
		{
			FInputModeUIOnly InputMode;
			if (UPRWidgetBase* Top = GetTopWidget())
			{
				InputMode.SetWidgetToFocus(Top->TakeWidget());
			}
			PC->SetInputMode(InputMode);
		}
		break;

	case EPBUIInputMode::GameAndUI:
		{
			FInputModeGameAndUI InputMode;
			InputMode.SetHideCursorDuringCapture(false);
			PC->SetInputMode(InputMode);
		}
		break;

	case EPBUIInputMode::None:
	default:
		{
			FInputModeGameOnly InputMode;
			PC->SetInputMode(InputMode);
		}
		break;
	}

	PC->bShowMouseCursor = bEffectiveCursor;
}

int32 UPRUIManagerSubsystem::FindLayerInsertIndex(EPRUILayer Layer) const
{
	// 같은 레이어 내에서는 새 위젯이 더 위에 오도록, 더 높은 레이어보다는 아래에 오도록 삽입
	for (int32 i = UIStack.Num() - 1; i >= 0; --i)
	{
		if (IsValid(UIStack[i]) && UIStack[i]->Layer <= Layer)
		{
			return i + 1;
		}
	}
	return 0;
}

int32 UPRUIManagerSubsystem::GetZOrderForLayer(EPRUILayer Layer) const
{
	// 레이어당 100 단위 간격. 같은 레이어 내에서는 AddToViewport 호출 순서로 자연스럽게 적층
	constexpr int32 LayerZStep = 100;
	return static_cast<int32>(Layer) * LayerZStep;
}

void UPRUIManagerSubsystem::CleanupInvalidEntries()
{
	UIStack.RemoveAll([](const TObjectPtr<UPRWidgetBase>& Widget)
	{
		return !IsValid(Widget) || !Widget->IsInViewport();
	});
}

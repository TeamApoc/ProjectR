// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (Party Health List UI 위젯 구현)
#include "ProjectR/UI/HUD/PRPartyHealthListWidget.h"

#include "AbilitySystemComponent.h"
#include "Components/PanelWidget.h"
#include "GameFramework/PlayerController.h"
#include "ProjectR/Game/PRGameStateBase.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "ProjectR/UI/HUD/PRPartyMemberHealthWidget.h"

UPRPartyHealthListWidget::UPRPartyHealthListWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Layer = EPRUILayer::HUD;
	InputMode = EPBUIInputMode::None;
	bShowMouseCursor = false;
}

/*~ UUserWidget Interface ~*/

void UPRPartyHealthListWidget::NativeConstruct()
{
	Super::NativeConstruct();

	CachePartyMemberSlots();
	BindPlayerCountChanged();
	RefreshPartyMembers();
}

void UPRPartyHealthListWidget::NativeDestruct()
{
	UnbindPlayerCountChanged();

	ApplyPartyMembers(TArray<APRPlayerState*>());

	Super::NativeDestruct();
}

/*~ Public API ~*/

void UPRPartyHealthListWidget::RefreshPartyMembers()
{
	UWorld* World = GetWorld();
	APRGameStateBase* GameState = IsValid(World) ? World->GetGameState<APRGameStateBase>() : nullptr;
	APRPlayerState* OwningPlayerState = GetOwningPRPlayerState();
	if (!IsValid(GameState) || !IsValid(OwningPlayerState))
	{
		ApplyPartyMembers(TArray<APRPlayerState*>());
		return;
	}

	TArray<APRPlayerState*> PartyMembers;
	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		APRPlayerState* PRPlayerState = Cast<APRPlayerState>(PlayerState);
		if (!IsValid(PRPlayerState) || PRPlayerState == OwningPlayerState || !IsValid(PRPlayerState->GetAbilitySystemComponent()))
		{
			continue;
		}

		PartyMembers.Add(PRPlayerState);
	}

	PartyMembers.Sort([](const APRPlayerState& Left, const APRPlayerState& Right)
	{
		const int32 LeftPlayerIndex = Left.GetPRPlayerIndex();
		const int32 RightPlayerIndex = Right.GetPRPlayerIndex();
		if (LeftPlayerIndex != INDEX_NONE && RightPlayerIndex != INDEX_NONE)
		{
			return LeftPlayerIndex < RightPlayerIndex;
		}

		return Left.GetPlayerId() < Right.GetPlayerId();
	});

	if (PartyMembers.Num() > PartyMemberSlots.Num())
	{
		PartyMembers.SetNum(PartyMemberSlots.Num());
	}

	ApplyPartyMembers(PartyMembers);
}

/*~ Party Members ~*/

void UPRPartyHealthListWidget::CachePartyMemberSlots()
{
	PartyMemberSlots.Reset();

	if (!IsValid(PartyMemberListPanel))
	{
		return;
	}

	const int32 ChildCount = PartyMemberListPanel->GetChildrenCount();
	PartyMemberSlots.Reserve(ChildCount);
	for (int32 ChildIndex = 0; ChildIndex < ChildCount; ++ChildIndex)
	{
		UPRPartyMemberHealthWidget* PartyMemberSlot = Cast<UPRPartyMemberHealthWidget>(PartyMemberListPanel->GetChildAt(ChildIndex));
		if (!IsValid(PartyMemberSlot))
		{
			continue;
		}

		PartyMemberSlots.Add(PartyMemberSlot);
	}
}

void UPRPartyHealthListWidget::BindPlayerCountChanged()
{
	if (PlayerCountChangedDelegateHandle.IsValid())
	{
		return;
	}

	UWorld* World = GetWorld();
	APRGameStateBase* GameState = IsValid(World) ? World->GetGameState<APRGameStateBase>() : nullptr;
	if (!IsValid(GameState))
	{
		return;
	}

	PlayerCountChangedDelegateHandle = GameState->OnPlayerCountChanged.AddUObject(this, &ThisClass::HandlePlayerCountChanged);
}

void UPRPartyHealthListWidget::UnbindPlayerCountChanged()
{
	if (!PlayerCountChangedDelegateHandle.IsValid())
	{
		return;
	}

	UWorld* World = GetWorld();
	APRGameStateBase* GameState = IsValid(World) ? World->GetGameState<APRGameStateBase>() : nullptr;
	if (IsValid(GameState))
	{
		GameState->OnPlayerCountChanged.Remove(PlayerCountChangedDelegateHandle);
	}

	PlayerCountChangedDelegateHandle.Reset();
}

void UPRPartyHealthListWidget::HandlePlayerCountChanged()
{
	RefreshPartyMembers();
}

APRPlayerState* UPRPartyHealthListWidget::GetOwningPRPlayerState() const
{
	APlayerController* OwningPlayer = GetOwningPlayer();
	return IsValid(OwningPlayer) ? OwningPlayer->GetPlayerState<APRPlayerState>() : nullptr;
}

void UPRPartyHealthListWidget::ApplyPartyMembers(const TArray<APRPlayerState*>& PartyMembers)
{
	for (int32 Index = 0; Index < PartyMemberSlots.Num(); ++Index)
	{
		UPRPartyMemberHealthWidget* PartySlot = PartyMemberSlots[Index];
		if (!IsValid(PartySlot))
		{
			continue;
		}

		if (PartyMembers.IsValidIndex(Index))
		{
			PartySlot->SetPlayerState(PartyMembers[Index]);
		}
		else
		{
			PartySlot->ClearPlayerState();
		}
	}

	const bool bHasPartyMembers = PartyMembers.Num() > 0;
	SetVisibility(bHasPartyMembers ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#include "ProjectR/UI/HUD/PRPartyHealthListWidget.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "ProjectR/UI/HUD/PRPartyMemberHealthWidget.h"
#include "TimerManager.h"

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

	RefreshPartyMembers();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			PartyRefreshTimerHandle,
			this,
			&UPRPartyHealthListWidget::RefreshPartyMembers,
			PartyRefreshInterval,
			true);
	}
}

void UPRPartyHealthListWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PartyRefreshTimerHandle);
	}

	ApplyPartyMembers(TArray<APRPlayerState*>());

	Super::NativeDestruct();
}

/*~ Public API ~*/

void UPRPartyHealthListWidget::RefreshPartyMembers()
{
	UWorld* World = GetWorld();
	AGameStateBase* GameState = IsValid(World) ? World->GetGameState() : nullptr;
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
		return Left.GetPlayerId() < Right.GetPlayerId();
	});

	if (PartyMembers.Num() > 2)
	{
		PartyMembers.SetNum(2);
	}

	ApplyPartyMembers(PartyMembers);
}

/*~ Party Members ~*/

APRPlayerState* UPRPartyHealthListWidget::GetOwningPRPlayerState() const
{
	APlayerController* OwningPlayer = GetOwningPlayer();
	return IsValid(OwningPlayer) ? OwningPlayer->GetPlayerState<APRPlayerState>() : nullptr;
}

void UPRPartyHealthListWidget::ApplyPartyMembers(const TArray<APRPlayerState*>& PartyMembers)
{
	UPRPartyMemberHealthWidget* Slots[] = { PartyMemberSlot0.Get(), PartyMemberSlot1.Get() };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Slots); ++Index)
	{
		UPRPartyMemberHealthWidget* PartySlot = Slots[Index];
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

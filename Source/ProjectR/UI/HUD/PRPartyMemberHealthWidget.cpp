// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#include "ProjectR/UI/HUD/PRPartyMemberHealthWidget.h"

#include "AbilitySystemComponent.h"
#include "Components/TextBlock.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "ProjectR/UI/HUD/PRHealthBarWidget.h"

UPRPartyMemberHealthWidget::UPRPartyMemberHealthWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Layer = EPRUILayer::HUD;
	InputMode = EPBUIInputMode::None;
	bShowMouseCursor = false;
}

/*~ UUserWidget Interface ~*/

void UPRPartyMemberHealthWidget::NativeDestruct()
{
	ClearPlayerState();

	Super::NativeDestruct();
}

void UPRPartyMemberHealthWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (BoundPlayerState.IsValid())
	{
		RefreshSurvivalState();
	}
}

/*~ Public API ~*/

void UPRPartyMemberHealthWidget::SetPlayerState(APRPlayerState* InPlayerState)
{
	if (!IsValid(InPlayerState))
	{
		ClearPlayerState();
		return;
	}

	if (BoundPlayerState.Get() == InPlayerState)
	{
		RefreshDisplayName();
		RefreshSurvivalState();
		return;
	}

	BoundPlayerState = InPlayerState;

	if (IsValid(HealthBar))
	{
		HealthBar->SetPresentationMode(EPRHealthBarPresentationMode::PartyMember);
		HealthBar->InitializeFromPlayerState(InPlayerState);
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);
	RefreshDisplayName();
	RefreshSurvivalState();
}

void UPRPartyMemberHealthWidget::ClearPlayerState()
{
	BoundPlayerState.Reset();

	if (IsValid(HealthBar))
	{
		HealthBar->ClearHealthSource();
	}

	SetVisibility(ESlateVisibility::Collapsed);
}

/*~ Display ~*/

void UPRPartyMemberHealthWidget::RefreshDisplayName()
{
	APRPlayerState* PlayerState = BoundPlayerState.Get();
	if (!IsValid(PlayerState))
	{
		return;
	}

	FString DisplayName = PlayerState->GetDisplayName();
	if (DisplayName.IsEmpty())
	{
		DisplayName = PlayerState->GetPlayerName();
	}

	const FText DisplayNameTextValue = FText::FromString(DisplayName);
	if (IsValid(DisplayNameText))
	{
		DisplayNameText->SetText(DisplayNameTextValue);
	}
	BP_OnDisplayNameChanged(DisplayNameTextValue);
}

void UPRPartyMemberHealthWidget::RefreshSurvivalState()
{
	APRPlayerState* PlayerState = BoundPlayerState.Get();
	UAbilitySystemComponent* AbilitySystemComponent = IsValid(PlayerState) ? PlayerState->GetAbilitySystemComponent() : nullptr;
	if (!IsValid(AbilitySystemComponent))
	{
		return;
	}

	EPRPartyMemberSurvivalState NewState = EPRPartyMemberSurvivalState::Alive;
	if (AbilitySystemComponent->HasMatchingGameplayTag(PRGameplayTags::State_Dead))
	{
		NewState = EPRPartyMemberSurvivalState::Dead;
	}
	else if (AbilitySystemComponent->HasMatchingGameplayTag(PRGameplayTags::State_Down))
	{
		NewState = EPRPartyMemberSurvivalState::Down;
	}

	if (SurvivalState != NewState)
	{
		SurvivalState = NewState;
		BP_OnSurvivalStateChanged(SurvivalState);
	}
}

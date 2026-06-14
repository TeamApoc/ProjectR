// Copyright ProjectR. All Rights Reserved.

#include "PRFaerinEncounterChoiceWidget.h"

#include "ProjectR/AI/Boss/Faerin/PRFaerinEncounterDirector.h"
#include "ProjectR/Player/PRPlayerController.h"

UPRFaerinEncounterChoiceWidget::UPRFaerinEncounterChoiceWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Layer = EPRUILayer::Modal;
	InputMode = EPBUIInputMode::UIOnly;
	bShowMouseCursor = true;
}

void UPRFaerinEncounterChoiceWidget::InitializeChoice(APRFaerinEncounterDirector* InDirector)
{
	EncounterDirector = InDirector;
	BP_OnChoiceInitialized(InDirector);
}

FReply UPRFaerinEncounterChoiceWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (GetUIInputAction(InKeyEvent.GetKey()) == EPRUIInputAction::Cancel)
	{
		RequestDecline();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UPRFaerinEncounterChoiceWidget::RequestFight()
{
	APRPlayerController* PlayerController = Cast<APRPlayerController>(GetOwningPlayer());
	if (!IsValid(PlayerController) || !IsValid(EncounterDirector.Get()))
	{
		return;
	}

	PlayerController->ServerChooseFaerinEncounterFight(EncounterDirector);
}

void UPRFaerinEncounterChoiceWidget::RequestDecline()
{
	APRPlayerController* PlayerController = Cast<APRPlayerController>(GetOwningPlayer());
	if (!IsValid(PlayerController) || !IsValid(EncounterDirector.Get()))
	{
		return;
	}

	PlayerController->ServerChooseFaerinEncounterDecline(EncounterDirector);
}

// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRPlayerMenuTabListWidget.h"

#include "CommonButtonBase.h"
#include "Components/HorizontalBox.h"
#include "ProjectR/UI/TextButton/PRTextButton.h"

void UPRPlayerMenuTabListWidget::HandleTabCreation_Implementation(FName TabNameID, UCommonButtonBase* TabButton)
{
	Super::HandleTabCreation_Implementation(TabNameID, TabButton);

	if (!IsValid(TabButtonContainer))
	{
		return;
	}

	if (!IsValid(TabButton))
	{
		return;
	}

	UPRTextButton* TabTextButton = Cast<UPRTextButton>(TabButton);
	if (!IsValid(TabTextButton))
	{
		return;
	}


	TabTextButton->SetText(FText::FromName(TabNameID));
	TabButtonContainer->AddChild(TabTextButton);
}

void UPRPlayerMenuTabListWidget::HandleTabRemoval_Implementation(FName TabNameID, UCommonButtonBase* TabButton)
{
	Super::HandleTabRemoval_Implementation(TabNameID, TabButton);

	if (!IsValid(TabButtonContainer))
	{
		return;
	}

	TabButtonContainer->RemoveChild(TabButton);
}

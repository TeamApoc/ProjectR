// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRPlayerMenuTabListWidget.h"

#include "Blueprint/WidgetTree.h"
#include "CommonButtonBase.h"
#include "Components/HorizontalBox.h"
#include "ProjectR/UI/TextButton/PRTextButton.h"

void UPRPlayerMenuTabListWidget::RebuildDesignPreviewTabs(const TArray<FName>& TabNameIDs, TSubclassOf<UCommonButtonBase> ButtonWidgetType)
{
	if (!IsDesignTime() || !IsValid(TabButtonContainer))
	{
		return;
	}

	TabButtonContainer->ClearChildren();
	if (!IsValid(ButtonWidgetType.Get()) || !IsValid(WidgetTree))
	{
		return;
	}

	for (const FName TabNameID : TabNameIDs)
	{
		UCommonButtonBase* PreviewTabButton = WidgetTree->ConstructWidget<UCommonButtonBase>(ButtonWidgetType);
		if (!IsValid(PreviewTabButton))
		{
			continue;
		}

		UPRTextButton* PreviewTextButton = Cast<UPRTextButton>(PreviewTabButton);
		if (!IsValid(PreviewTextButton))
		{
			TabButtonContainer->AddChild(PreviewTabButton);
			continue;
		}

		// 디자인 탭 텍스트
		PreviewTextButton->SetText(FText::FromName(TabNameID));
		TabButtonContainer->AddChild(PreviewTextButton);
	}
}

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

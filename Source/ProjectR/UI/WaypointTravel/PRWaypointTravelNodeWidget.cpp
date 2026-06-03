// Copyright ProjectR. All Rights Reserved.

#include "PRWaypointTravelNodeWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"

void UPRWaypointTravelNodeWidget::SetNodeOption(const FPRWaypointTravelNodeOption& InNodeOption)
{
	NodeOption = InNodeOption;
	RefreshNodeText();
}

void UPRWaypointTravelNodeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 목적지 버튼 초기화
	BindChildWidgetEvents();
	RefreshNodeText();
}

void UPRWaypointTravelNodeWidget::NativeDestruct()
{
	// 목적지 버튼 정리
	UnbindChildWidgetEvents();

	Super::NativeDestruct();
}

void UPRWaypointTravelNodeWidget::BindChildWidgetEvents()
{
	if (IsValid(NodeButton))
	{
		// 버튼 클릭 수신
		NodeButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleNodeButtonClicked);
		NodeButton->OnClicked.AddDynamic(this, &ThisClass::HandleNodeButtonClicked);
	}
}

void UPRWaypointTravelNodeWidget::UnbindChildWidgetEvents()
{
	if (IsValid(NodeButton))
	{
		// 버튼 클릭 해제
		NodeButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleNodeButtonClicked);
	}
}

void UPRWaypointTravelNodeWidget::RefreshNodeText()
{
	if (IsValid(WorldNameText))
	{
		// 맵 이름 표시
		WorldNameText->SetText(NodeOption.WorldDisplayName);
	}

	if (IsValid(WaypointNameText))
	{
		// 웨이포인트 이름 표시
		WaypointNameText->SetText(NodeOption.WaypointDisplayName);
	}
}

void UPRWaypointTravelNodeWidget::HandleNodeButtonClicked()
{
	// 노드 선택 이벤트 전달
	OnNodeSelected.Broadcast(NodeOption);
}

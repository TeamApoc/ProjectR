// Copyright ProjectR. All Rights Reserved.

#include "PRWaypointTravelWidget.h"

#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "ProjectR/Player/PRPlayerController.h"
#include "ProjectR/UI/Components/PRUIControllerComponent.h"
#include "ProjectR/UI/WaypointTravel/PRWaypointTravelNodeWidget.h"
#include "ProjectR/World/PRWorldDataAsset.h"

UPRWaypointTravelWidget::UPRWaypointTravelWidget()
{
	Layer = EPRUILayer::Menu;
	InputMode = EPBUIInputMode::UIOnly;
	bShowMouseCursor = true;
}

void UPRWaypointTravelWidget::RebuildNodeList()
{
	NodeWidgets.Reset();

	if (!IsValid(NodeListPanel))
	{
		return;
	}

	NodeListPanel->ClearChildren();

	if (!IsValid(NodeWidgetClass.Get()))
	{
		return;
	}

	APlayerController* OwningPlayerController = GetOwningPlayer();
	if (!IsValid(OwningPlayerController))
	{
		return;
	}

	const TArray<FPRWaypointTravelNodeOption> NodeOptions = BuildTemporaryNodeOptions();
	for (const FPRWaypointTravelNodeOption& NodeOption : NodeOptions)
	{
		UPRWaypointTravelNodeWidget* NodeWidget = CreateWidget<UPRWaypointTravelNodeWidget>(
			OwningPlayerController,
			NodeWidgetClass);
		if (!IsValid(NodeWidget))
		{
			continue;
		}

		// 목적지 버튼 생성
		NodeWidget->SetNodeOption(NodeOption);
		NodeWidget->OnNodeSelected.RemoveDynamic(this, &ThisClass::HandleNodeSelected);
		NodeWidget->OnNodeSelected.AddDynamic(this, &ThisClass::HandleNodeSelected);
		NodeListPanel->AddChild(NodeWidget);
		NodeWidgets.Add(NodeWidget);
	}
}

void UPRWaypointTravelWidget::RequestTravelToNode(const FPRWaypointTravelNodeOption& NodeOption)
{
	if (bTravelRequestPending)
	{
		return;
	}

	APRPlayerController* PlayerController = GetOwningPlayer<APRPlayerController>();
	if (!IsValid(PlayerController))
	{
		return;
	}

	// 서버 이동 요청
	bTravelRequestPending = true;
	PlayerController->RequestWaypointTravel(NodeOption.WorldDataAsset.ToSoftObjectPath(), NodeOption.WaypointId);
}

void UPRWaypointTravelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Travel UI 초기화
	bTravelRequestPending = false;
	BindChildWidgetEvents();
	RebuildNodeList();
}

void UPRWaypointTravelWidget::NativeDestruct()
{
	// Travel UI 정리
	UnbindChildWidgetEvents();
	NodeWidgets.Reset();

	Super::NativeDestruct();
}

FReply UPRWaypointTravelWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (GetUIInputAction(InKeyEvent.GetKey()) == EPRUIInputAction::Cancel)
	{
		// 취소 입력 처리
		CloseTravelWidget(true);
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UPRWaypointTravelWidget::BindChildWidgetEvents()
{
	if (IsValid(CloseButton))
	{
		// 닫기 입력 수신
		CloseButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleCloseButtonClicked);
		CloseButton->OnClicked.AddDynamic(this, &ThisClass::HandleCloseButtonClicked);
	}
}

void UPRWaypointTravelWidget::UnbindChildWidgetEvents()
{
	if (IsValid(CloseButton))
	{
		// 닫기 입력 해제
		CloseButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleCloseButtonClicked);
	}

	for (UPRWaypointTravelNodeWidget* NodeWidget : NodeWidgets)
	{
		if (IsValid(NodeWidget))
		{
			// 노드 선택 해제
			NodeWidget->OnNodeSelected.RemoveDynamic(this, &ThisClass::HandleNodeSelected);
		}
	}
}

TArray<FPRWaypointTravelNodeOption> UPRWaypointTravelWidget::BuildTemporaryNodeOptions() const
{
	TArray<FPRWaypointTravelNodeOption> NodeOptions;

	for (const TSoftObjectPtr<UPRWorldDataAsset>& WorldDataAssetPtr : TemporaryWorldDataAssets)
	{
		UPRWorldDataAsset* WorldDataAsset = WorldDataAssetPtr.LoadSynchronous();
		if (!IsValid(WorldDataAsset))
		{
			continue;
		}

		for (const FPRWaypointTravelNodeDefinition& NodeDefinition : WorldDataAsset->WaypointNodes)
		{
			if (!NodeDefinition.WaypointId.IsValid())
			{
				continue;
			}

			// 임시 노드 변환
			FPRWaypointTravelNodeOption NodeOption;
			NodeOption.WorldDataAsset = WorldDataAssetPtr;
			NodeOption.MapAsset = WorldDataAsset->MapAsset;
			NodeOption.WaypointId = NodeDefinition.WaypointId;
			NodeOption.WorldDisplayName = WorldDataAsset->DisplayName;
			NodeOption.WaypointDisplayName = NodeDefinition.DisplayName;
			NodeOptions.Add(NodeOption);
		}
	}

	return NodeOptions;
}

void UPRWaypointTravelWidget::CloseTravelWidget(bool bNotifyServerCancel)
{
	APRPlayerController* PlayerController = GetOwningPlayer<APRPlayerController>();
	if (bNotifyServerCancel && !bTravelRequestPending && IsValid(PlayerController))
	{
		// 서버 취소 요청
		PlayerController->RequestCancelWaypointTravel();
	}

	if (!IsValid(PlayerController))
	{
		return;
	}

	UPRUIControllerComponent* UIController = PlayerController->GetUIController();
	if (IsValid(UIController))
	{
		// 로컬 UI 닫기
		UIController->CloseWaypointTravel();
	}
}

void UPRWaypointTravelWidget::HandleCloseButtonClicked()
{
	// 닫기 버튼 처리
	CloseTravelWidget(true);
}

void UPRWaypointTravelWidget::HandleNodeSelected(const FPRWaypointTravelNodeOption& NodeOption)
{
	// 노드 선택 처리
	RequestTravelToNode(NodeOption);
}

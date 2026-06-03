// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/UI/PRWidgetBase.h"
#include "ProjectR/UI/WaypointTravel/PRWaypointTravelTypes.h"
#include "PRWaypointTravelWidget.generated.h"

class UButton;
class UPanelWidget;
class UPRWaypointTravelNodeWidget;
class UPRWorldDataAsset;

// 웨이포인트 목적지 노드 목록을 표시하는 호스트 전용 Travel UI
UCLASS(Abstract, BlueprintType)
class PROJECTR_API UPRWaypointTravelWidget : public UPRWidgetBase
{
	GENERATED_BODY()

public:
	UPRWaypointTravelWidget();

	// 임시 월드 데이터 에셋 배열 기준 목적지 노드 목록 재생성
	UFUNCTION(BlueprintCallable, Category = "ProjectR|WaypointTravel")
	void RebuildNodeList();

	// 선택한 목적지 노드 이동 요청 전송
	UFUNCTION(BlueprintCallable, Category = "ProjectR|WaypointTravel")
	void RequestTravelToNode(const FPRWaypointTravelNodeOption& NodeOption);

protected:
	/*~ UUserWidget Interface ~*/
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	// 하위 위젯 이벤트 바인딩
	void BindChildWidgetEvents();

	// 하위 위젯 이벤트 바인딩 해제
	void UnbindChildWidgetEvents();

	// 임시 월드 데이터 에셋 배열에서 목적지 노드 목록 생성, TODO: 세이브 기반 노드 활성화
	TArray<FPRWaypointTravelNodeOption> BuildTemporaryNodeOptions() const;

	// Travel UI 닫기 처리
	void CloseTravelWidget(bool bNotifyServerCancel);

	// 닫기 버튼 클릭 처리
	UFUNCTION()
	void HandleCloseButtonClicked();

	// 목적지 노드 선택 처리
	UFUNCTION()
	void HandleNodeSelected(const FPRWaypointTravelNodeOption& NodeOption);

protected:
	// 개발 단계에서 UI에 표시할 월드 데이터 에셋 목록
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|WaypointTravel")
	TArray<TSoftObjectPtr<UPRWorldDataAsset>> TemporaryWorldDataAssets;

	// 목적지 노드 버튼으로 생성할 위젯 클래스
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|WaypointTravel")
	TSubclassOf<UPRWaypointTravelNodeWidget> NodeWidgetClass;

	// UMG에서 바인딩할 목적지 노드 목록 패널
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|WaypointTravel")
	TObjectPtr<UPanelWidget> NodeListPanel;

	// UMG에서 바인딩할 닫기 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|WaypointTravel")
	TObjectPtr<UButton> CloseButton;

private:
	// 동적으로 생성한 목적지 노드 버튼 목록
	UPROPERTY(Transient)
	TArray<TObjectPtr<UPRWaypointTravelNodeWidget>> NodeWidgets;

	// 서버 이동 요청 대기 상태
	bool bTravelRequestPending = false;
};

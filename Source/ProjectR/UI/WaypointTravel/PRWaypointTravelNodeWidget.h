// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/UI/PRWidgetBase.h"
#include "ProjectR/UI/WaypointTravel/PRWaypointTravelTypes.h"
#include "PRWaypointTravelNodeWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPRWaypointTravelNodeSelectedSignature, const FPRWaypointTravelNodeOption&, NodeOption);

// 웨이포인트 Travel UI의 단일 목적지 버튼
UCLASS(Abstract, BlueprintType)
class PROJECTR_API UPRWaypointTravelNodeWidget : public UPRWidgetBase
{
	GENERATED_BODY()

public:
	// 버튼 표시와 요청에 사용할 목적지 노드 지정
	UFUNCTION(BlueprintCallable, Category = "ProjectR|WaypointTravel")
	void SetNodeOption(const FPRWaypointTravelNodeOption& InNodeOption);

	// 현재 버튼 목적지 노드 반환
	UFUNCTION(BlueprintPure, Category = "ProjectR|WaypointTravel")
	const FPRWaypointTravelNodeOption& GetNodeOption() const { return NodeOption; }

protected:
	/*~ UUserWidget Interface ~*/
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	// 하위 위젯 이벤트 바인딩
	void BindChildWidgetEvents();

	// 하위 위젯 이벤트 바인딩 해제
	void UnbindChildWidgetEvents();

	// 목적지 이름 표시 갱신
	void RefreshNodeText();

	// 버튼 클릭 처리
	UFUNCTION()
	void HandleNodeButtonClicked();

public:
	// 목적지 버튼 클릭 이벤트
	UPROPERTY(BlueprintAssignable, Category = "ProjectR|WaypointTravel")
	FPRWaypointTravelNodeSelectedSignature OnNodeSelected;

protected:
	// UMG에서 바인딩할 목적지 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|WaypointTravel")
	TObjectPtr<UButton> NodeButton;

	// UMG에서 바인딩할 맵 이름 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|WaypointTravel")
	TObjectPtr<UTextBlock> WorldNameText;

	// UMG에서 바인딩할 웨이포인트 이름 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|WaypointTravel")
	TObjectPtr<UTextBlock> WaypointNameText;

private:
	// 현재 버튼이 요청할 목적지 노드
	UPROPERTY(Transient, BlueprintReadOnly, Category = "ProjectR|WaypointTravel", meta = (AllowPrivateAccess = "true"))
	FPRWaypointTravelNodeOption NodeOption;
};

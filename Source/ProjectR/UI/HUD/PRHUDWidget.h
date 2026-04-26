// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/UI/PRWidgetBase.h"
#include "PRHUDWidget.generated.h"

class UPRCrosshairWidget;
class UPREventManagerSubsystem;
struct FInstancedStruct;
struct FGameplayTag;

/**
 * 인게임 HUD 컨테이너 위젯.
 * BP UMG에서 자식 위젯들을 BindWidgetOptional 로 구성하며,
 * 자식 존재 여부에 따라 EventManager 이벤트 바인딩을 자동으로 처리한다.
 */
UCLASS(Abstract, BlueprintType)
class PROJECTR_API UPRHUDWidget : public UPRWidgetBase
{
	GENERATED_BODY()

public:
	UPRHUDWidget();

protected:
	/*~ UUserWidget Interface ~*/
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

private:
	// EventManager 콜백: 에이밍 시작 - 크로스헤어 표시
	void HandleAimStart(FGameplayTag EventTag, const FInstancedStruct& Payload);

	// EventManager 콜백: 에이밍 종료 - 크로스헤어 숨김
	void HandleAimEnd(FGameplayTag EventTag, const FInstancedStruct& Payload);

	// 등록된 EventManager 핸들 정리
	void UnbindFromEventManager();

	// 크로스헤어 가시성 토글
	void SetCrosshairVisible(bool bVisible);

	// 현재 월드의 EventManager 조회
	UPREventManagerSubsystem* GetEventManager() const;

protected:
	// UMG 트리에서 동일 이름("CrosshairWidget")의 자식이 있을 때 자동 바인딩.
	// BP 레이아웃에 크로스헤어가 없으면 nullptr이며, 이 경우 에임 이벤트 바인딩도 건너뛴다
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "HUD")
	TObjectPtr<UPRCrosshairWidget> CrosshairWidget;

private:
	FDelegateHandle AimStartHandle;
	FDelegateHandle AimEndHandle;
};

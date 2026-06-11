// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "PRPlayerMenu.generated.h"

class UCommonButtonBase;
class UPRPlayerMenuTabListWidget;
class UCommonActivatableWidgetSwitcher;
/**
 *
 */
UCLASS()
class PROJECTR_API UPRPlayerMenu : public UCommonActivatableWidget
{
	GENERATED_BODY()
public:
	// 플레이어 메뉴 기본 입력 설정 초기화
	UPRPlayerMenu();

	// 디자인 타임 미리보기 처리
	virtual void NativePreConstruct() override;

	// 탭 목록 초기 등록
	virtual void NativeOnInitialized() override;

	// 플레이어 메뉴 닫기 입력 처리
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|PlayerMenu")
	TObjectPtr<UCommonActivatableWidgetSwitcher> WidgetSwitcher;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|PlayerMenu")
	TObjectPtr<UPRPlayerMenuTabListWidget> TabList;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|PlayerMenu")
	TSubclassOf<UCommonButtonBase> TabButtonClass;

private:
	// 스위처 자식 위젯을 탭 목록에 등록
	void RegisterSwitcherTabs();
};

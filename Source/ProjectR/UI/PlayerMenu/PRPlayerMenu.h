// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/UI/PRWidgetBase.h"
#include "PRPlayerMenu.generated.h"

class UCommonButtonBase;
class UCommonTabListWidgetBase;
class UCommonActivatableWidgetSwitcher;
/**
 *
 */
UCLASS()
class PROJECTR_API UPRPlayerMenu : public UPRWidgetBase
{
	GENERATED_BODY()
public:
	// 플레이어 메뉴 기본 입력 설정 초기화
	UPRPlayerMenu();

	// 디자인 타임 미리보기 처리
	virtual void NativePreConstruct() override;

	// 런타임 탭 목록 갱신
	bool RefreshRuntimeTabs();

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|PlayerMenu")
	TObjectPtr<UCommonActivatableWidgetSwitcher> WidgetSwitcher;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|PlayerMenu")
	TObjectPtr<UCommonTabListWidgetBase> TabList;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|PlayerMenu")
	TSubclassOf<UCommonButtonBase> TabButtonClass;

};

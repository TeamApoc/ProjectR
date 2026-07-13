// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonTabListWidgetBase.h"

#include "PRPlayerMenuTabListWidget.generated.h"

class UHorizontalBox;
class UCommonButtonBase;
class UCommonActivatableWidgetSwitcher;
/**
 * 
 */
UCLASS()
class PROJECTR_API UPRPlayerMenuTabListWidget : public UCommonTabListWidgetBase
{
	GENERATED_BODY()

public:
	// 탭 목록 갱신
	bool RefreshTabs(UCommonActivatableWidgetSwitcher* InWidgetSwitcher, TSubclassOf<UCommonButtonBase> ButtonWidgetType, int32 DesiredTabIndex);
	
protected:
	/*~ UCommonTabListWidgetBase Interface ~*/
	virtual void HandleTabCreation_Implementation(FName TabNameID, UCommonButtonBase* TabButton) override;
	
	virtual void HandleTabRemoval_Implementation(FName TabNameID, UCommonButtonBase* TabButton) override;

private:
	// 탭 버튼 표시 반영
	void AddTabButtonToContainer(FName TabNameID, UCommonButtonBase* TabButton);
	
protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|TabList")
	TObjectPtr<UHorizontalBox> TabButtonContainer;
};

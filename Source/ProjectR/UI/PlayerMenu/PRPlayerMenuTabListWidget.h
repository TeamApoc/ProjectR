// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonTabListWidgetBase.h"

#include "PRPlayerMenuTabListWidget.generated.h"

class UHorizontalBox;
class UCommonActionWidget;
/**
 * 
 */
UCLASS()
class PROJECTR_API UPRPlayerMenuTabListWidget : public UCommonTabListWidgetBase
{
	GENERATED_BODY()
	
protected:
	virtual void HandleTabCreation_Implementation(FName TabNameID, UCommonButtonBase* TabButton) override;
	
	virtual void HandleTabRemoval_Implementation(FName TabNameID, UCommonButtonBase* TabButton) override;
	
protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|TabList")
	TObjectPtr<UCommonActionWidget> TabLeft;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|TabList")
	TObjectPtr<UCommonActionWidget> TabRight;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|TabList")
	TObjectPtr<UHorizontalBox> TabButtonContainer;
};

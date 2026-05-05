// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/UI/Inventory/PRInventoryUITypes.h"
#include "ProjectR/UI/PRWidgetBase.h"
#include "PRItemTooltipWidget.generated.h"

class UImage;
class UTextBlock;

// 아이템 슬롯에 마우스를 올렸을 때 표시할 상세 정보 위젯이다
UCLASS(Abstract, BlueprintType)
class PROJECTR_API UPRItemTooltipWidget : public UPRWidgetBase
{
	GENERATED_BODY()

public:
	// 툴팁 표시 데이터를 갱신한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Inventory")
	void SetTooltipViewData(const FPRInventoryItemSlotViewData& InViewData);

	// 현재 툴팁 표시 데이터를 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|Inventory")
	FPRInventoryItemSlotViewData GetTooltipViewData() const;

private:
	// 현재 표시 데이터를 네이티브 바인딩 위젯에 반영한다
	void RefreshNativeDisplay();

	// 아이템 타입 표시 텍스트를 만든다
	FText GetItemTypeText() const;

protected:
	// UMG에서 바인딩할 아이템 이름 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Inventory")
	TObjectPtr<UTextBlock> ItemNameText;

	// UMG에서 바인딩할 아이템 타입 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Inventory")
	TObjectPtr<UTextBlock> ItemTypeText;

	// UMG에서 바인딩할 아이템 아이콘 이미지
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Inventory")
	TObjectPtr<UImage> ItemIconImage;

	// UMG에서 바인딩할 상세 설명 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Inventory")
	TObjectPtr<UTextBlock> DescriptionText;

private:
	// 현재 툴팁 표시 데이터
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Inventory", meta = (AllowPrivateAccess = "true"))
	FPRInventoryItemSlotViewData ViewData;
};

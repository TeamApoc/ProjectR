// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/Shop/Types/PRShopTypes.h"
#include "ProjectR/UI/PRWidgetBase.h"
#include "PRShopItemDetailWidget.generated.h"

class UImage;
class UTextBlock;

// 상점에서 선택된 아이템의 상세 정보를 표시하는 위젯
UCLASS(Abstract, BlueprintType)
class PROJECTR_API UPRShopItemDetailWidget : public UPRWidgetBase
{
	GENERATED_BODY()

public:
	// 선택 아이템 상세 표시 데이터를 갱신
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Shop")
	void SetSelectedItemDetails(const FPRShopItemSlotViewData& InViewData, EPRShopTabType InCurrentTabType, int32 InOwnedStackCount);

protected:
	/*~ UUserWidget Interface ~*/
	// 초기화 시 현재 상세 표시 데이터를 반영
	virtual void NativeOnInitialized() override;

private:
	// 현재 상세 표시 데이터를 네이티브 바인딩 위젯에 반영
	void RefreshNativeDisplay();

	// 재고 표시 텍스트를 만든다
	FText MakeStockText(bool bHasSelectedItem) const;

protected:
	// UMG에서 바인딩할 선택 아이템 이름 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Shop")
	TObjectPtr<UTextBlock> SelectedItemNameText;

	// UMG에서 바인딩할 선택 아이템 설명 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Shop")
	TObjectPtr<UTextBlock> SelectedItemDescriptionText;

	// UMG에서 바인딩할 선택 아이템 아이콘 이미지
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Shop")
	TObjectPtr<UImage> SelectedItemIconImage;

	// UMG에서 바인딩할 선택 아이템 가격 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Shop")
	TObjectPtr<UTextBlock> SelectedItemPriceText;

	// UMG에서 바인딩할 선택 아이템 재고 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Shop")
	TObjectPtr<UTextBlock> SelectedItemStockText;

	// UMG에서 바인딩할 선택 아이템 보유 수량 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Shop")
	TObjectPtr<UTextBlock> SelectedItemOwnedCountText;

private:
	// 현재 선택된 상점 아이템 표시 데이터
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Shop", meta = (AllowPrivateAccess = "true"))
	FPRShopItemSlotViewData ViewData;

	// 현재 상점 탭
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Shop", meta = (AllowPrivateAccess = "true"))
	EPRShopTabType CurrentTabType = EPRShopTabType::Buy;

	// 현재 플레이어가 보유한 선택 아이템 수량
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Shop", meta = (AllowPrivateAccess = "true"))
	int32 OwnedStackCount = 0;
};

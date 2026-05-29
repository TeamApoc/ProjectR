// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ProjectR/UI/Inventory/PRInventoryUITypes.h"
#include "PRItemTooltipViewDataBuilder.generated.h"

// 아이템 툴팁 표시 데이터 생성 빌더
UCLASS()
class PROJECTR_API UPRItemTooltipViewDataBuilder : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// 슬롯 표시 데이터를 아이템 툴팁 표시 데이터로 변환
	UFUNCTION(BlueprintPure, Category = "ProjectR|Inventory|Tooltip")
	static FPRItemTooltipViewData BuildTooltipViewData(const FPRInventoryItemSlotViewData& SlotViewData);
};

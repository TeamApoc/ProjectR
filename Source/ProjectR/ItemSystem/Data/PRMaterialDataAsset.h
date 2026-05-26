// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PRItemDataAsset.h"
#include "PRMaterialDataAsset.generated.h"

// 강화와 제작 비용에 사용하는 재료 아이템 데이터다
UCLASS(BlueprintType)
class PROJECTR_API UPRMaterialDataAsset : public UPRItemDataAsset
{
	GENERATED_BODY()

public:
	// 재료 Item 타입과 기본 스택 수량을 초기화한다
	UPRMaterialDataAsset();
};

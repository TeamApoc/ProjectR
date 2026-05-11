// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/Inventory/Types/PRItemTypes.h"
#include "ProjectR/AbilitySystem/Data/PRAbilitySet.h"
#include "PRItemDataAsset.h"
#include "PRConsumableDataAsset.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTR_API UPRConsumableDataAsset : public UPRItemDataAsset
{
	GENERATED_BODY()
	
public:
	UPRConsumableDataAsset();
	
public:
	// 아이템 사용 시 적용하는 GE 목록
	UPROPERTY(EditAnywhere, Category = "ProjectR|01_Consumable")
	TArray<FPREffectEntry> UseItemEffects;
	
	// 아이템 최대 보유 개수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|01_Consumable", meta = (ClampMin = "1"))
	int32 MaxStackCount = 99;
};

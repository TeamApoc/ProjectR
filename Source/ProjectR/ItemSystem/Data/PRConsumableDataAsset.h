// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "PRItemDataAsset.h"
#include "PRConsumableDataAsset.generated.h"

class UPRGA_UseConsumable;

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
	// 소비 아이템 사용 시 실행할 어빌리티 클래스
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|01_Consumable")
	TSubclassOf<UPRGA_UseConsumable> UseAbilityClass;
};

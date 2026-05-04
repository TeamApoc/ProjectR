// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/Inventory/Types/PRItemTypes.h"
#include "PRInventoryUITypes.generated.h"

class UPRItemDataAsset;
class UPRItemInstance;
class UTexture2D;

UENUM(BlueprintType)
enum class EPRInventoryItemListType : uint8
{
	None,
	Weapon,
	Mod
};

// 인벤토리 UI에서 아이템 한 칸을 표시하기 위한 데이터다
USTRUCT(BlueprintType)
struct PROJECTR_API FPRInventoryItemSlotViewData
{
	GENERATED_BODY()

public:
	// 표시할 아이템 공통 데이터
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Inventory")
	TObjectPtr<UPRItemDataAsset> ItemData = nullptr;

	// 선택 시 장착 요청에 사용할 아이템 인스턴스
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Inventory")
	TObjectPtr<UPRItemInstance> ItemInstance = nullptr;

	// UI에 표시할 아이템 이름
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Inventory")
	FText DisplayName;

	// UI에 표시할 아이템 아이콘
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Inventory")
	TObjectPtr<UTexture2D> Icon = nullptr;

	// 아이템 분류
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Inventory")
	EPRItemType ItemType = EPRItemType::None;

	// 현재 장착 상태인지 여부
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Inventory")
	bool bEquipped = false;

	// 장착 해제 항목인지 여부
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Inventory")
	bool bUnequipEntry = false;
};


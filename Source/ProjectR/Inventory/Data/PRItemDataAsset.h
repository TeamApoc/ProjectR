// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ProjectR/Inventory/Types/PRItemTypes.h"
#include "PRItemDataAsset.generated.h"

class UTexture2D;

// 아이템 공통 표시 데이터를 담는 기반 데이터 에셋이다
UCLASS(Abstract, BlueprintType)
class PROJECTR_API UPRItemDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPRItemDataAsset();

public:
	// 아이템 표시 이름을 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|00_Item")
	const FText& GetDisplayName() const { return DisplayName; }

	// 아이템 아이콘을 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|00_Item")
	UTexture2D* GetIcon() const { return Icon; }

	// 아이템 타입을 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|00_Item")
	EPRItemType GetItemType() const { return ItemType; }

protected:
	// 파생 데이터 에셋 생성자에서 아이템 타입을 고정한다
	void SetItemType(EPRItemType NewItemType);

protected:
	// UI에 표시할 아이템 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|00_Item")
	FText DisplayName;

	// UI에 표시할 아이템 아이콘
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|00_Item")
	TObjectPtr<UTexture2D> Icon = nullptr;

private:
	// 아이템 타입
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|00_Item", meta = (AllowPrivateAccess = "true"))
	EPRItemType ItemType = EPRItemType::None;
};

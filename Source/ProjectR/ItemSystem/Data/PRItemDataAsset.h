// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ProjectR/ItemSystem/Types/PRItemTypes.h"
#include "PRItemDataAsset.generated.h"

class UTexture2D;
class UStaticMesh;

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

	// 아이템 설명을 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|00_Item")
	const FText& GetDescription() const { return Description; }

	// 아이템 타입을 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|00_Item")
	EPRItemType GetItemType() const { return ItemType; }

	// 아이템 희귀도를 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|00_Item")
	EPRItemRarity GetRarity() const { return Rarity; }

	// 월드 픽업에 표시할 메시를 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|00_Item")
	UStaticMesh* GetPickupMesh() const { return PickupMesh; }

	// 월드 픽업에 표시할 메시 스케일을 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|00_Item")
	const FVector& GetPickupMeshScale() const { return PickupMeshScale; }

protected:
	// 파생 데이터 에셋 생성자에서 아이템 타입을 고정한다
	void SetItemType(EPRItemType NewItemType);

public:
	// UI에 표시할 아이템 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|00_Item")
	FText DisplayName;

	// UI에 표시할 아이템 아이콘
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|00_Item")
	TObjectPtr<UTexture2D> Icon = nullptr;

	// UI 상세 패널에 표시할 아이템 설명
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|00_Item", meta = (MultiLine = "true"))
	FText Description;
	
	// 아이템 최대 보유 개수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|01_Consumable", meta = (ClampMin = "1"))
	int32 MaxStackCount = 1;

	// 아이템 희귀도
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|00_Item")
	EPRItemRarity Rarity = EPRItemRarity::Common;

	// 월드 픽업에 표시할 메시
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|00_Item")
	TObjectPtr<UStaticMesh> PickupMesh = nullptr;

	// 월드 픽업에 표시할 메시 스케일
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|00_Item", meta = (ClampMin = "0.0"))
	FVector PickupMeshScale = FVector::OneVector;

private:
	// 아이템 타입
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|00_Item", meta = (AllowPrivateAccess = "true"))
	EPRItemType ItemType = EPRItemType::None;
};

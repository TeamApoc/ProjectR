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

	// PickupMesh 소켓 기준 위치 오프셋 반환
	UFUNCTION(BlueprintPure, Category = "Item|Consumable")
	const FVector& GetPickupMeshSocketLocationOffset() const { return PickupMeshSocketLocationOffset; }

	// PickupMesh 소켓 기준 회전 오프셋 반환
	UFUNCTION(BlueprintPure, Category = "Item|Consumable")
	const FRotator& GetPickupMeshSocketRotationOffset() const { return PickupMeshSocketRotationOffset; }

public:
	// 소비 아이템 사용 시 실행할 어빌리티 클래스
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Consumable")
	TSubclassOf<UPRGA_UseConsumable> UseAbilityClass;

	// PickupMesh 소켓 기준 위치 오프셋
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Consumable|Visual")
	FVector PickupMeshSocketLocationOffset = FVector::ZeroVector;

	// PickupMesh 소켓 기준 회전 오프셋
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Consumable|Visual")
	FRotator PickupMeshSocketRotationOffset = FRotator::ZeroRotator;
};

// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/ItemSystem/Data/PRItemDataAsset.h"
#include "ProjectR/ItemSystem/Types/PREquipmentTypes.h"
#include "PREquipmentDataAsset.generated.h"

class USkeletalMesh;

// 장비 아이템의 정적 데이터를 정의
UCLASS(BlueprintType)
class PROJECTR_API UPREquipmentDataAsset : public UPRItemDataAsset
{
	GENERATED_BODY()

public:
	UPREquipmentDataAsset();

	// 장착 슬롯 반환
	EPREquipmentSlotType GetSlotType() const { return SlotType; }

	// 장비 외형 메시 반환
	TSoftObjectPtr<USkeletalMesh> GetEquipmentMesh() const { return EquipmentMesh; }

protected:
	// 이 장비가 장착되는 슬롯 종류
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Equipment")
	EPREquipmentSlotType SlotType = EPREquipmentSlotType::None;

	// 장착 시 캐릭터에 부착될 메시 (추후 ChildMesh로 사용)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Equipment")
	TSoftObjectPtr<USkeletalMesh> EquipmentMesh;
};

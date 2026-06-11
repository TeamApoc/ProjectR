// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (방어구 파츠 스켈레탈 메시 및 능력치 데이터 정의)
// Author: 손승우 (보스전용 특수 장착 에셋 데이터 연동)
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

	// 플레이어 얼굴 숨김 여부 반환
	bool ShouldHidePlayerFace() const { return SlotType == EPREquipmentSlotType::Head && bHidePlayerFace; }

protected:
	// 이 장비가 장착되는 슬롯 종류
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Equipment")
	EPREquipmentSlotType SlotType = EPREquipmentSlotType::None;

	// 장착 시 캐릭터에 부착될 메시 (추후 ChildMesh로 사용)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Equipment")
	TSoftObjectPtr<USkeletalMesh> EquipmentMesh;

	// 머리 장비 장착 시 플레이어 얼굴 메시 숨김 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Equipment", meta = (EditCondition = "SlotType == EPREquipmentSlotType::Head", EditConditionHides))
	bool bHidePlayerFace = false;
};

// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProjectR/Game/PRGameTypes.h"
#include "PREquipmentManagerComponent.generated.h"

// 방어구와 악세서리 같은 비무기 장착 요소의 책임 경계를 잡는 컴포넌트다
UCLASS(ClassGroup = (ProjectR), meta = (BlueprintSpawnableComponent))
class PROJECTR_API UPREquipmentManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPREquipmentManagerComponent();

	// 현재 비무기 장비 상태 저장 데이터
	FPREquipmentSaveData MakeSaveData() const;

	// 저장 데이터 기반 비무기 장비 상태 복원
	void ApplySaveData(const FPREquipmentSaveData& InSaveData);
};

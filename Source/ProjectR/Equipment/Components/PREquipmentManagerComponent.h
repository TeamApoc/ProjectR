// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PREquipmentManagerComponent.generated.h"

// 방어구와 악세서리 같은 비무기 장착 요소의 책임 경계를 잡는 컴포넌트다
UCLASS(ClassGroup = (ProjectR), meta = (BlueprintSpawnableComponent))
class PROJECTR_API UPREquipmentManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPREquipmentManagerComponent();
};

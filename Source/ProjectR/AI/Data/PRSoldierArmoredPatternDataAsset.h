// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AI/Data/PRPatternDataAsset.h"
#include "PRSoldierArmoredPatternDataAsset.generated.h"

// Soldier_Armored 전용 패턴 DataAsset 타입
// 실제 패턴 후보, 거리, 가중치는 에디터 작성 기준
UCLASS(BlueprintType)
class PROJECTR_API UPRSoldierArmoredPatternDataAsset : public UPRPatternDataAsset
{
	GENERATED_BODY()

public:
	UPRSoldierArmoredPatternDataAsset();
};

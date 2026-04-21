// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AI/Data/PRPatternDataAsset.h"
#include "PRSoldierArmoredPatternDataAsset.generated.h"

// Soldier_Armored의 기본 패턴 후보를 코드 기본값으로 제공한다.
// 에디터의 DA_SoldierArmoredPattern에서 필요하면 거리/가중치를 덮어쓸 수 있다.
UCLASS(BlueprintType)
class PROJECTR_API UPRSoldierArmoredPatternDataAsset : public UPRPatternDataAsset
{
	GENERATED_BODY()

public:
	UPRSoldierArmoredPatternDataAsset();
};

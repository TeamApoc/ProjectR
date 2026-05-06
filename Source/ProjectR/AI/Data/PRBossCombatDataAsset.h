// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AI/Data/PREnemyCombatDataAsset.h"
#include "PRBossCombatDataAsset.generated.h"

// 보스 몬스터가 공통으로 사용하는 전투 데이터 자산이다.
// 개별 보스 전용 수치가 필요해지면 이 클래스를 다시 상속해 확장한다.
UCLASS(BlueprintType)
class PROJECTR_API UPRBossCombatDataAsset : public UPREnemyCombatDataAsset
{
	GENERATED_BODY()
};

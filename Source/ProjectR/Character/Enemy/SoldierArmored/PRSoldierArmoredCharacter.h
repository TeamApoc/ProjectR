// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/Character/Enemy/PREnemyBaseCharacter.h"
#include "PRSoldierArmoredCharacter.generated.h"

class UMotionWarpingComponent;
class UPRSoldierArmoredDebugDrawComponent;

// Fae Soldier Armored 전용 캐릭터 클래스
// 기본 이동값, 피격 충돌, Motion Warping 컴포넌트를 관리
UCLASS()
class PROJECTR_API APRSoldierArmoredCharacter : public APREnemyBaseCharacter
{
	GENERATED_BODY()

public:
	APRSoldierArmoredCharacter();

protected:
	// Soldier_Armored PIE 디버그 범위 출력
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|Debug")
	TObjectPtr<UPRSoldierArmoredDebugDrawComponent> DebugDrawComponent;

	// 에디터 Notify State에서 사용하는 Motion Warping 컴포넌트
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|Animation")
	TObjectPtr<UMotionWarpingComponent> MotionWarpingComponent;
};

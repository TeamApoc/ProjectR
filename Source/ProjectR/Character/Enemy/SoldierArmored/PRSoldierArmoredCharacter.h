// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/Character/Enemy/PREnemyBaseCharacter.h"
#include "PRSoldierArmoredCharacter.generated.h"

// Fae Soldier Armored 전용 캐릭터 클래스다.
// 공통 EnemyBase 위에 CharacterID, 이동 속도, 갑옷/약점 충돌 기본값을 지정한다.
UCLASS()
class PROJECTR_API APRSoldierArmoredCharacter : public APREnemyBaseCharacter
{
	GENERATED_BODY()

public:
	APRSoldierArmoredCharacter();
};

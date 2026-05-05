// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/Character/Enemy/PRBossBaseCharacter.h"
#include "PRFaerinCharacter.generated.h"

// Faerin 보스 본체 클래스다.
// 패턴 분기와 포털/검 생성 로직은 넣지 않고, 보스 공통 베이스에 Faerin 기본 데이터만 얹는다.
UCLASS(Blueprintable)
class PROJECTR_API APRFaerinCharacter : public APRBossBaseCharacter
{
	GENERATED_BODY()

public:
	APRFaerinCharacter();
};

// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/Character/Enemy/PREnemyBaseCharacter.h"
#include "PRPenitentCharacter.generated.h"

UCLASS()
class PROJECTR_API APRPenitentCharacter : public APREnemyBaseCharacter
{
	GENERATED_BODY()

public:
	// Penitent 캐릭터 기본값을 초기화한다.
	APRPenitentCharacter();
};

// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "PRPlayHUD.generated.h"

/**
 * 인게임 플레이용 HUD 셸.
 * 실제 HUD 위젯 생성·관리는 PlayerController의 UPRUIControllerComponent가 담당한다.
 * GameMode HUDClass 호환을 위해 클래스만 유지한다.
 */
UCLASS()
class PROJECTR_API APRPlayHUD : public AHUD
{
	GENERATED_BODY()

public:
	APRPlayHUD();
};

// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PRMenuGameMode.generated.h"

// 메인 메뉴 전용 GameMode. 세션 없음. 로비 UI와 Host/Join 입력만 담당
// 인게임 로직은 APRPlayGameMode에서 수행한다
UCLASS()
class PROJECTR_API APRMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	APRMenuGameMode();
};

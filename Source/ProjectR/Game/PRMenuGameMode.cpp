// Copyright ProjectR. All Rights Reserved.

#include "PRMenuGameMode.h"

#include "ProjectR/Game/PRMenuHUD.h"
#include "ProjectR/Player/PRMenuPlayerController.h"

APRMenuGameMode::APRMenuGameMode()
{
	// 메뉴는 복제 불필요. GameMode 기본값 유지
	bUseSeamlessTravel = true;

	// 메뉴 플레이어 입력과 HUD 표시 클래스 지정
	PlayerControllerClass = APRMenuPlayerController::StaticClass();
	HUDClass = APRMenuHUD::StaticClass();
}

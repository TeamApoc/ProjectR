// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (Menu HUD 구현)
#include "PRMenuHUD.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "ProjectR/Player/PRMenuPlayerController.h"
#include "ProjectR/UI/StartMenu/PRStartMenuWidget.h"

void APRMenuHUD::BeginPlay()
{
	Super::BeginPlay();

	if (!IsValid(PlayerOwner) || !PlayerOwner->IsLocalController())
	{
		return;
	}

	if (!IsValid(StartMenuWidgetClass.Get()))
	{
		UE_LOG(LogTemp, Warning, TEXT("[MenuHUD] 시작 메뉴 위젯 클래스 설정 없음"));
		return;
	}

	// 시작 메뉴 위젯 생성과 뷰포트 추가
	StartMenuWidget = CreateWidget<UPRStartMenuWidget>(PlayerOwner, StartMenuWidgetClass);
	if (!IsValid(StartMenuWidget))
	{
		UE_LOG(LogTemp, Warning, TEXT("[MenuHUD] 시작 메뉴 위젯 생성 실패"));
		return;
	}

	StartMenuWidget->AddToViewport();

	if (APRMenuPlayerController* MenuPlayerController = Cast<APRMenuPlayerController>(PlayerOwner))
	{
		// 생성된 시작 메뉴 위젯 기준 입력 포커스 적용
		MenuPlayerController->ApplyMenuInputMode(StartMenuWidget);
	}
}

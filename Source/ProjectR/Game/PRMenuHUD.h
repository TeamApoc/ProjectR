// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (Menu HUD 구현)
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "PRMenuHUD.generated.h"

class UPRStartMenuWidget;

// 메인 메뉴 전용 HUD. 시작 메뉴 위젯 생성과 뷰포트 추가를 담당함
UCLASS()
class PROJECTR_API APRMenuHUD : public AHUD
{
	GENERATED_BODY()

protected:
	/*~ AActor Interface ~*/
	// HUD 시작 시 로컬 플레이어 뷰포트에 시작 메뉴 위젯 추가
	virtual void BeginPlay() override;

protected:
	// 메뉴 월드에서 표시할 시작 메뉴 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Menu")
	TSubclassOf<UPRStartMenuWidget> StartMenuWidgetClass;

private:
	// 현재 표시 중인 시작 메뉴 위젯
	UPROPERTY(Transient)
	TObjectPtr<UPRStartMenuWidget> StartMenuWidget;
};

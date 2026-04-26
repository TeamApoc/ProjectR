// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "PRPlayHUD.generated.h"

class UPRHUDWidget;
class UPRUIManagerSubsystem;

/**
 * 인게임 플레이용 HUD.
 * BeginPlay에서 HUDWidget 인스턴스를 생성하고, UIManager 를 통해 Push 한다.
 * 자식 위젯(크로스헤어 등)의 이벤트 처리는 HUDWidget 내부에서 담당한다.
 */
UCLASS()
class PROJECTR_API APRPlayHUD : public AHUD
{
	GENERATED_BODY()

public:
	APRPlayHUD();

protected:
	/*~ AActor Interface ~*/
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// LocalPlayer의 UI 매니저 조회
	UPRUIManagerSubsystem* GetUIManager() const;

protected:
	// 사용할 HUD 위젯 클래스. 기본값은 BP 파생 클래스에서 지정
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|HUD")
	TSubclassOf<UPRHUDWidget> HUDWidgetClass;

private:
	UPROPERTY(Transient)
	TObjectPtr<UPRHUDWidget> HUDWidget;
};

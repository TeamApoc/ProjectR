// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "PRDeveloperSettings.generated.h"

enum class EPRCrosshairType : uint8;
class UPRAbilitySystemRegistry;
class UUserWidget;

// 프로젝트 전역 Registry 에셋 지정용 DeveloperSettings. 프로젝트 설정 > Game > ProjectR 에서 편집
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "ProjectR"))
class PROJECTR_API UPRDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/*~ UDeveloperSettings Interface ~*/
	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	// CrosshairType에 따른 CrosshairWidgetClass를 동기 로드 후 반환
	TSubclassOf<UUserWidget> GetCrosshairWidgetSync(EPRCrosshairType CrosshairType) const;
	
public:
	// AbilitySystem 전용 Registry 소프트 레퍼런스
	UPROPERTY(EditAnywhere, Config, Category = "Registries", meta = (AllowedClasses = "/Script/ProjectR.PRAbilitySystemRegistry"))
	TSoftObjectPtr<UPRAbilitySystemRegistry> AbilitySystemRegistry;
	
	// EPRCrosshairType : CrosshairWidgetClass 매핑
	UPROPERTY(EditAnywhere, Config, Category = "Registries")
	TMap<EPRCrosshairType, TSoftClassPtr<UUserWidget>> CrosshairWidgets;
};

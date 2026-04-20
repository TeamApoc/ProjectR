// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "PRDeveloperSettings.generated.h"

class UPRAbilitySystemRegistry;

// 프로젝트 전역 Registry 에셋 지정용 DeveloperSettings. 프로젝트 설정 > Game > ProjectR 에서 편집
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "ProjectR"))
class PROJECTR_API UPRDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/*~ UDeveloperSettings Interface ~*/
	virtual FName GetCategoryName() const override { return TEXT("Game"); }

public:
	// AbilitySystem 전용 Registry 소프트 레퍼런스
	UPROPERTY(EditAnywhere, Config, Category = "Registries", meta = (AllowedClasses = "/Script/ProjectR.PRAbilitySystemRegistry"))
	TSoftObjectPtr<UPRAbilitySystemRegistry> AbilitySystemRegistry;
};

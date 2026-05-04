// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "PRDeveloperSettings.generated.h"

enum class EPRCrosshairType : uint8;
enum class EPRFloatingTextType : uint8;
class UPRAbilitySystemRegistry;
class UPRFloatingTextWidget;
class UUserWidget;

// 플로팅 텍스트 타입별 위젯 클래스 및 색상 설정
USTRUCT(BlueprintType)
struct FPRFloatingTextStyleConfig
{
	GENERATED_BODY()

	// 사용할 위젯 클래스
	UPROPERTY(EditAnywhere, Config, Category = "FloatingText")
	TSoftClassPtr<UPRFloatingTextWidget> WidgetClass;

	// 텍스트 색상
	UPROPERTY(EditAnywhere, Config, Category = "FloatingText")
	FLinearColor Color = FLinearColor::White;
};

// 위젯 클래스가 로드된 결과. Getter 반환용
struct FPRFloatingTextStyle
{
	// 로드 완료된 위젯 클래스
	TSubclassOf<UPRFloatingTextWidget> WidgetClass;

	// 텍스트 색상
	FLinearColor Color = FLinearColor::White;
};

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

	// FloatingTextType에 따른 스타일(위젯 클래스 + 색상)을 동기 로드 후 반환
	FPRFloatingTextStyle GetFloatingTextStyleSync(EPRFloatingTextType TextType) const;
	
public:
	// AbilitySystem 전용 Registry 소프트 레퍼런스
	UPROPERTY(EditAnywhere, Config, Category = "Registries", meta = (AllowedClasses = "/Script/ProjectR.PRAbilitySystemRegistry"))
	TSoftObjectPtr<UPRAbilitySystemRegistry> AbilitySystemRegistry;
	
	// EPRCrosshairType : CrosshairWidgetClass 매핑
	UPROPERTY(EditAnywhere, Config, Category = "Registries")
	TMap<EPRCrosshairType, TSoftClassPtr<UUserWidget>> CrosshairWidgets;

	// EPRFloatingTextType : 스타일(위젯 + 색상) 매핑
	UPROPERTY(EditAnywhere, Config, Category = "Registries")
	TMap<EPRFloatingTextType, FPRFloatingTextStyleConfig> FloatingTextStyles;
};

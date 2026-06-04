// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ProjectR/UI/WorldMarker/PRWorldMarkerTypes.h"
#include "PRDeveloperSettings.generated.h"

enum class EPRCrosshairType : uint8;
enum class EPRFloatingTextType : uint8;
class UPRAbilitySystemRegistry;
class UPRFXRegistryDataAsset;
class UPRFloatingTextWidget;
class UGameplayEffect;
class UUserWidget;
class UDataTable;
class APRRewardPickupActor;

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

	// 레이어 Z-Order 값
	UPROPERTY(EditAnywhere, Config, Category = "FloatingText")
	int32 LayerZOrder = 0;
};

// 위젯 클래스가 로드된 결과. Getter 반환용
struct FPRFloatingTextStyle
{
	// 로드 완료된 위젯 클래스
	TSubclassOf<UPRFloatingTextWidget> WidgetClass;

	// 텍스트 색상
	FLinearColor Color = FLinearColor::White;

	// 레이어 Z-Order 값
	int32 LayerZOrder = 0;
};

UENUM(BlueprintType)
enum class EPRWorldMarkerPreset : uint8
{
	Default,
	Enemy,
	Interactable
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

	// 타입에 맞는 WorldMarker 프리셋 반환
	FPRWorldMarkerVisualData GetWorldMarkerPreset(EPRWorldMarkerPreset InPresetType) const;

public:
	// AbilitySystem 전용 Registry 소프트 레퍼런스
	UPROPERTY(EditAnywhere, Config, Category = "Registries", meta = (AllowedClasses = "/Script/ProjectR.PRAbilitySystemRegistry"))
	TSoftObjectPtr<UPRAbilitySystemRegistry> AbilitySystemRegistry;

	// FX 태그와 Cue 클래스를 연결하는 Registry 소프트 레퍼런스
	UPROPERTY(EditAnywhere, Config, Category = "Registries", meta = (AllowedClasses = "/Script/ProjectR.PRFXRegistryDataAsset"))
	TSoftObjectPtr<UPRFXRegistryDataAsset> FXRegistry;

	// 몬스터별 드롭 보상 테이블
	UPROPERTY(EditAnywhere, Config, Category = "Registries", meta = (RequiredAssetDataTags = "RowStructure=/Script/ProjectR.PRMonsterDropTableRow"))
	TSoftObjectPtr<UDataTable> MonsterDropTable;

	// 레벨별 누적 필요 경험치 테이블
	UPROPERTY(EditAnywhere, Config, Category = "Registries", meta = (RequiredAssetDataTags = "RowStructure=/Script/ProjectR.PRLevelExperienceRow"))
	TSoftObjectPtr<UDataTable> LevelExperienceTable;

	// 특성별 투자 공식 테이블
	UPROPERTY(EditAnywhere, Config, Category = "Registries", meta = (RequiredAssetDataTags = "RowStructure=/Script/ProjectR.PRTraitStatRuleRow"))
	TSoftObjectPtr<UDataTable> TraitStatRuleTable;

	// 특성 보너스 적용에 사용할 에디터 작성 GameplayEffect
	UPROPERTY(EditAnywhere, Config, Category = "Registries", meta = (AllowedClasses = "/Script/GameplayAbilities.GameplayEffect"))
	TSoftClassPtr<UGameplayEffect> TraitBonusEffectClass;

	// 드롭 보상 픽업에 사용할 액터 클래스
	UPROPERTY(EditAnywhere, Config, Category = "Registries", meta = (AllowedClasses = "/Script/ProjectR.PRRewardPickupActor"))
	TSoftClassPtr<APRRewardPickupActor> RewardPickupActorClass;
	
	// EPRCrosshairType : CrosshairWidgetClass 매핑
	UPROPERTY(EditAnywhere, Config, Category = "UI")
	TMap<EPRCrosshairType, TSoftClassPtr<UUserWidget>> CrosshairWidgets;

	// EPRFloatingTextType : 스타일(위젯 + 색상) 매핑
	UPROPERTY(EditAnywhere, Config, Category = "UI")
	TMap<EPRFloatingTextType, FPRFloatingTextStyleConfig> FloatingTextStyles;

	// 기본 월드 마커 스타일
	UPROPERTY(EditDefaultsOnly, Config,  Category = "UI|WorldMarker")
	TMap<EPRWorldMarkerPreset, FPRWorldMarkerVisualData> WorldMarkerPresets;

	// 기본 월드 마커 유지 시간
	UPROPERTY(EditDefaultsOnly, Config,  Category = "UI|WorldMarker", meta = (ClampMin = "0.0"))
	float DefaultWorldMarkerDuration = 12.0f;

	// 거리 텍스트 표시 시작 거리
	UPROPERTY(EditDefaultsOnly, Config, Category = "UI|WorldMarker", meta = (ClampMin = "0.0", Units = "m"))
	float WorldMarkerDistanceVisibleMinMeters = 20.0f;
};

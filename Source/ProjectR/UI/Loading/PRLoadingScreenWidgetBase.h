// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "PRLoadingScreenWidgetBase.generated.h"

class UImage;

UENUM(BlueprintType)
enum class EPRLoadingScreenWidgetPhase : uint8
{
	MoviePlayer,
	Viewport
};

USTRUCT(BlueprintType)
struct FPRLoadingScreenImageEntry
{
	GENERATED_BODY()

public:
	// 목적지 맵 패키지 이름 또는 짧은 맵 이름
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Loading")
	FName MapName = NAME_None;

	// 목적지 맵에 사용할 첫 번째 이미지
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Loading")
	FSlateBrush PrimaryImageBrush;

	// MoviePlayer 단계에서 사용할 두 번째 이미지
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Loading")
	FSlateBrush MoviePlayerSecondaryImageBrush;

	// Viewport 로딩 단계에서 사용할 두 번째 이미지
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Loading")
	FSlateBrush ViewportSecondaryImageBrush;
};

UCLASS(Blueprintable)
class PROJECTR_API UPRLoadingScreenWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	// 목적지 맵 이름을 설정하고 대응 이미지를 적용
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Loading")
	void SetLoadingDestination(const FString& MapPackageName);

	// 로딩 화면 표시 단계를 설정하고 두 번째 이미지를 갱신
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Loading")
	void SetLoadingScreenWidgetPhase(EPRLoadingScreenWidgetPhase NewPhase);

	// 목적지 맵 패키지 이름 반환
	UFUNCTION(BlueprintPure, Category = "ProjectR|Loading")
	FString GetDestinationMapPackageName() const { return DestinationMapPackageName; }

	// 목적지 맵 짧은 이름 반환
	UFUNCTION(BlueprintPure, Category = "ProjectR|Loading")
	FName GetDestinationMapShortName() const { return DestinationMapShortName; }

	// 현재 로딩 화면 표시 단계 반환
	UFUNCTION(BlueprintPure, Category = "ProjectR|Loading")
	EPRLoadingScreenWidgetPhase GetLoadingScreenWidgetPhase() const { return LoadingScreenWidgetPhase; }

protected:
	// 목적지 맵 변경 시 블루프린트 후처리
	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|Loading")
	void OnLoadingDestinationChanged(const FString& MapPackageName, FName MapShortName);

private:
	const FPRLoadingScreenImageEntry* FindImageEntry(FName MapPackageName, FName MapShortName) const;
	void ApplyImageBrushes(const FPRLoadingScreenImageEntry* ImageEntry);
	FName ResolveShortMapName(const FString& MapPackageName) const;

protected:
	// 첫 번째 이미지 위젯
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Loading")
	TObjectPtr<UImage> PrimaryImage;

	// 두 번째 이미지 위젯
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Loading")
	TObjectPtr<UImage> SecondaryImage;

	// 매핑이 없을 때 사용할 첫 번째 이미지
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Loading")
	FSlateBrush DefaultPrimaryImageBrush;

	// 매핑이 없을 때 MoviePlayer 단계에서 사용할 두 번째 이미지
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Loading")
	FSlateBrush DefaultMoviePlayerSecondaryImageBrush;

	// 매핑이 없을 때 Viewport 로딩 단계에서 사용할 두 번째 이미지
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Loading")
	FSlateBrush DefaultViewportSecondaryImageBrush;

	// 목적지 맵별 이미지 매핑
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Loading")
	TArray<FPRLoadingScreenImageEntry> MapImageEntries;

private:
	UPROPERTY(Transient)
	FString DestinationMapPackageName;

	UPROPERTY(Transient)
	FName DestinationMapShortName = NAME_None;

	UPROPERTY(Transient)
	EPRLoadingScreenWidgetPhase LoadingScreenWidgetPhase = EPRLoadingScreenWidgetPhase::Viewport;
};

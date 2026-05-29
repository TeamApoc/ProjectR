// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/Game/PRGameTypes.h"
#include "PRAssetUtils.generated.h"

class APRPlayerCharacter;
class UPRWeaponManagerComponent;

// 에셋 경로 수집 유틸리티
UCLASS()
class PROJECTR_API UPRAssetUtils : public UObject
{
	GENERATED_BODY()

public:
	// 캐릭터 저장 데이터 프리뷰에 필요한 1차 에셋 경로 수집
	static void CollectPreviewRootAssetPaths(const FPRCharacterSaveData& SaveData, TArray<FSoftObjectPath>& OutAssetPaths);

	// 현재 캐릭터 프리뷰에 필요한 1차 에셋 경로 수집
	static void CollectPreviewRootAssetPaths(const APRPlayerCharacter* SourceCharacter, const UPRWeaponManagerComponent* WeaponManagerComponent, TArray<FSoftObjectPath>& OutAssetPaths);

	// 현재 캐릭터 프리뷰에서 이미 로드된 1차 에셋 수집
	static void CollectPreviewLoadedRootAssets(const APRPlayerCharacter* SourceCharacter, const UPRWeaponManagerComponent* WeaponManagerComponent, TArray<UObject*>& OutLoadedAssets);

	// 로드된 1차 에셋에서 프리뷰 적용에 필요한 의존 에셋 경로 수집
	static void CollectPreviewDependentAssetPaths(const TArray<UObject*>& LoadedAssets, TArray<FSoftObjectPath>& OutAssetPaths);

	// 비동기 로드 결과 맵에서 UObject 배열 수집
	static void CollectLoadedAssetsFromMap(const TMap<FSoftObjectPath, TWeakObjectPtr<UObject>>& LoadedAssetMap, TArray<UObject*>& OutLoadedAssets);

private:
	// 유효한 경로 중복 제거 추가
	static void AddUniqueAssetPath(const FSoftObjectPath& AssetPath, TArray<FSoftObjectPath>& OutAssetPaths);

	// 유효한 UObject 경로 중복 제거 추가
	static void AddUniqueAssetPathFromObject(const UObject* AssetObject, TArray<FSoftObjectPath>& OutAssetPaths);
};

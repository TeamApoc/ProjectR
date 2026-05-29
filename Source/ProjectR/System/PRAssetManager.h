// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "PRAssetManager.generated.h"

class UPRAbilitySystemRegistry;
class UDataTable;
class UPRItemDataAsset;
struct FStreamableHandle;
struct FPRMonsterDropTableRow;

// 비동기 로드 완료 결과
struct FPRAssetLoadResult
{
	// 요청 식별자
	uint64 RequestId = 0;

	// 요청 경로별 로드 결과
	TMap<FSoftObjectPath, TWeakObjectPtr<UObject>> LoadedAssets;
};

// 비동기 로드 완료 네이티브 콜백
DECLARE_DELEGATE_OneParam(FPRAssetsLoadedNative, const FPRAssetLoadResult&);

// 프로젝트 전용 AssetManager. DeveloperSettings에 지정된 Registry들을 시작 시 동기 로드 및 캐싱
UCLASS(Config = Game)
class PROJECTR_API UPRAssetManager : public UAssetManager
{
	GENERATED_BODY()

public:
	// 싱글톤 접근. 프로젝트 AssetManager 클래스 미지정 시 체크 실패
	static UPRAssetManager& Get();

	/*~ UAssetManager Interface ~*/
	virtual void StartInitialLoading() override;

public:
	// 캐싱된 AbilitySystem Registry 반환. 미로드 시 Lazy Load
	UPRAbilitySystemRegistry* GetAbilitySystemRegistry();

	// 몬스터 드롭 테이블 Row를 조회한다
	const FPRMonsterDropTableRow* FindMonsterDropRow(FName MonsterId);

	// Primary Asset Id로 아이템 데이터 에셋을 조회한다
	UPRItemDataAsset* GetItemDataByPrimaryAssetId(const FPrimaryAssetId& ItemAssetId);

	// UObject 경로 목록을 타입 해석 없이 비동기 로드 요청
	uint64 LoadAssetsAsync(const TArray<FSoftObjectPath>& AssetPaths, FPRAssetsLoadedNative Callback);

private:
	// DeveloperSettings 기반 Registry들을 동기 로드 및 캐시에 등록
	void LoadRegistries();

private:
	UPROPERTY(Transient)
	TObjectPtr<UPRAbilitySystemRegistry> CachedAbilitySystemRegistry;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> CachedMonsterDropTable;

	uint64 NextAsyncLoadRequestId = 1;

	TMap<uint64, TSharedPtr<FStreamableHandle>> ActiveAsyncLoadHandles;
};

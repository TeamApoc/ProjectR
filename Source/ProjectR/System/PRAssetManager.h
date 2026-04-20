// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "PRAssetManager.generated.h"

class UPRAbilitySystemRegistry;

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

private:
	// DeveloperSettings 기반 Registry들을 동기 로드 및 캐시에 등록
	void LoadRegistries();

private:
	UPROPERTY(Transient)
	TObjectPtr<UPRAbilitySystemRegistry> CachedAbilitySystemRegistry;
};

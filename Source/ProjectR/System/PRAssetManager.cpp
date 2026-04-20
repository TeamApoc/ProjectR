// Copyright ProjectR. All Rights Reserved.

#include "PRAssetManager.h"
#include "PRDeveloperSettings.h"
#include "ProjectR/AbilitySystem/Data/PRAbilitySystemRegistry.h"

UPRAssetManager& UPRAssetManager::Get()
{
	UPRAssetManager* Singleton = Cast<UPRAssetManager>(GEngine->AssetManager);
	checkf(Singleton, TEXT("PRAssetManager 미설정. DefaultEngine.ini의 AssetManagerClassName을 PRAssetManager로 지정 필요"));
	return *Singleton;
}

void UPRAssetManager::StartInitialLoading()
{
	Super::StartInitialLoading();
	LoadRegistries();
}

UPRAbilitySystemRegistry* UPRAssetManager::GetAbilitySystemRegistry()
{
	if (!CachedAbilitySystemRegistry)
	{
		LoadRegistries();
	}
	return CachedAbilitySystemRegistry;
}

void UPRAssetManager::LoadRegistries()
{
	const UPRDeveloperSettings* Settings = GetDefault<UPRDeveloperSettings>();
	if (Settings == nullptr)
	{
		return;
	}

	if (!CachedAbilitySystemRegistry && !Settings->AbilitySystemRegistry.IsNull())
	{
		CachedAbilitySystemRegistry = Settings->AbilitySystemRegistry.LoadSynchronous();
		if (!CachedAbilitySystemRegistry)
		{
			UE_LOG(LogTemp, Error, TEXT("PRAssetManager: AbilitySystemRegistry 로드 실패"));
		}
	}
}

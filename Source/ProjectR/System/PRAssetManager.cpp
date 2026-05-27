// Copyright ProjectR. All Rights Reserved.

#include "PRAssetManager.h"
#include "PRDeveloperSettings.h"
#include "Engine/DataTable.h"
#include "ProjectR/AbilitySystem/Data/PRAbilitySystemRegistry.h"
#include "ProjectR/ItemSystem/Data/PRItemDataAsset.h"
#include "ProjectR/ItemSystem/Types/PRDropTypes.h"

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

const FPRMonsterDropTableRow* UPRAssetManager::FindMonsterDropRow(FName MonsterId)
{
	if (MonsterId.IsNone())
	{
		return nullptr;
	}

	if (!CachedMonsterDropTable)
	{
		LoadRegistries();
	}

	if (!::IsValid(CachedMonsterDropTable))
	{
		return nullptr;
	}

	static const FString ContextString(TEXT("UPRAssetManager::FindMonsterDropRow"));
	return CachedMonsterDropTable->FindRow<FPRMonsterDropTableRow>(MonsterId, ContextString, false);
}

UPRItemDataAsset* UPRAssetManager::GetItemDataByPrimaryAssetId(const FPrimaryAssetId& ItemAssetId)
{
	if (!ItemAssetId.IsValid())
	{
		return nullptr;
	}

	const FSoftObjectPath AssetPath = GetPrimaryAssetPath(ItemAssetId);
	if (!AssetPath.IsValid())
	{
		return nullptr;
	}

	return Cast<UPRItemDataAsset>(AssetPath.TryLoad());
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

	if (!CachedMonsterDropTable && !Settings->MonsterDropTable.IsNull())
	{
		CachedMonsterDropTable = Settings->MonsterDropTable.LoadSynchronous();
		if (!CachedMonsterDropTable)
		{
			UE_LOG(LogTemp, Error, TEXT("PRAssetManager: MonsterDropTable 로드 실패"));
		}
	}
}

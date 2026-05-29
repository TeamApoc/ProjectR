// Copyright ProjectR. All Rights Reserved.

#include "PRAssetManager.h"
#include "PRDeveloperSettings.h"
#include "Engine/DataTable.h"
#include "Engine/StreamableManager.h"
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

uint64 UPRAssetManager::LoadAssetsAsync(const TArray<FSoftObjectPath>& AssetPaths, FPRAssetsLoadedNative Callback)
{
	const uint64 RequestId = NextAsyncLoadRequestId++;

	TArray<FSoftObjectPath> UniqueAssetPaths;
	for (const FSoftObjectPath& AssetPath : AssetPaths)
	{
		if (!AssetPath.IsValid())
		{
			continue;
		}

		// 중복 경로 제거
		UniqueAssetPaths.AddUnique(AssetPath);
	}

	if (UniqueAssetPaths.IsEmpty())
	{
		FPRAssetLoadResult Result;
		Result.RequestId = RequestId;
		Callback.ExecuteIfBound(Result);
		return RequestId;
	}

	TWeakObjectPtr<UPRAssetManager> WeakThis(this);
	TSharedPtr<FStreamableHandle> StreamableHandle = GetStreamableManager().RequestAsyncLoad(
		UniqueAssetPaths,
		FStreamableDelegate::CreateLambda([WeakThis, RequestId, UniqueAssetPaths, Callback]()
		{
			UPRAssetManager* AssetManager = WeakThis.Get();
			if (!AssetManager)
			{
				return;
			}

			FPRAssetLoadResult Result;
			Result.RequestId = RequestId;
			for (const FSoftObjectPath& AssetPath : UniqueAssetPaths)
			{
				// 로드 완료 경로의 UObject 해석
				Result.LoadedAssets.Add(AssetPath, AssetPath.ResolveObject());
			}

			Callback.ExecuteIfBound(Result);
			AssetManager->ActiveAsyncLoadHandles.Remove(RequestId);
		}));

	if (StreamableHandle.IsValid())
	{
		// 콜백 완료 전 핸들 보존
		ActiveAsyncLoadHandles.Add(RequestId, StreamableHandle);
	}
	else
	{
		FPRAssetLoadResult Result;
		Result.RequestId = RequestId;
		for (const FSoftObjectPath& AssetPath : UniqueAssetPaths)
		{
			// 즉시 해석 가능한 로드 결과
			Result.LoadedAssets.Add(AssetPath, AssetPath.ResolveObject());
		}

		Callback.ExecuteIfBound(Result);
	}

	return RequestId;
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

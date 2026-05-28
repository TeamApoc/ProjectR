// Copyright ProjectR. All Rights Reserved.

#include "PRAssetUtils.h"

#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/ItemSystem/Actors/PRWeaponActor.h"
#include "ProjectR/ItemSystem/Components/PREquipmentManagerComponent.h"
#include "ProjectR/ItemSystem/Components/PRWeaponManagerComponent.h"
#include "ProjectR/ItemSystem/Data/PREquipmentDataAsset.h"
#include "ProjectR/ItemSystem/Data/PRWeaponDataAsset.h"
#include "ProjectR/ItemSystem/Data/PRWeaponModDataAsset.h"

void UPRAssetUtils::CollectPreviewRootAssetPaths(const FPRCharacterSaveData& SaveData, TArray<FSoftObjectPath>& OutAssetPaths)
{
	for (const FPREquipmentSlotSaveEntry& EquippedSlot : SaveData.Equipment.EquippedSlots)
	{
		if (!SaveData.Inventory.Equipments.IsValidIndex(EquippedSlot.EquipmentItemIndex))
		{
			continue;
		}

		// 장착 장비 데이터 에셋 경로
		const FPREquipmentItemSaveEntry& EquipmentEntry = SaveData.Inventory.Equipments[EquippedSlot.EquipmentItemIndex];
		AddUniqueAssetPath(EquipmentEntry.EquipmentData.ToSoftObjectPath(), OutAssetPaths);
	}

	const int32 PreviewWeaponIndexes[] =
	{
		SaveData.WeaponManager.PrimaryWeaponIndex,
		SaveData.WeaponManager.SecondaryWeaponIndex
	};

	for (const int32 WeaponIndex : PreviewWeaponIndexes)
	{
		if (!SaveData.Inventory.Weapons.IsValidIndex(WeaponIndex))
		{
			continue;
		}

		// 장착 무기 데이터 에셋 경로
		const FPRWeaponItemSaveEntry& WeaponEntry = SaveData.Inventory.Weapons[WeaponIndex];
		AddUniqueAssetPath(WeaponEntry.WeaponData.ToSoftObjectPath(), OutAssetPaths);

		if (SaveData.Inventory.Mods.IsValidIndex(WeaponEntry.EquippedModIndex))
		{
			// 장착 Mod 데이터 에셋 경로
			const FPRModItemSaveEntry& ModEntry = SaveData.Inventory.Mods[WeaponEntry.EquippedModIndex];
			AddUniqueAssetPath(ModEntry.ModData.ToSoftObjectPath(), OutAssetPaths);
		}
	}
}

void UPRAssetUtils::CollectPreviewRootAssetPaths(const APRPlayerCharacter* SourceCharacter, const UPRWeaponManagerComponent* WeaponManagerComponent, TArray<FSoftObjectPath>& OutAssetPaths)
{
	if (IsValid(SourceCharacter))
	{
		if (const UPREquipmentManagerComponent* EquipmentManager = SourceCharacter->GetEquipmentManager())
		{
			for (const FPRReplicatedEquipmentInfo& EquipmentInfo : EquipmentManager->GetEquippedVisualInfos())
			{
				// 현재 장착 장비 데이터 에셋 경로
				AddUniqueAssetPath(EquipmentInfo.EquipmentData.ToSoftObjectPath(), OutAssetPaths);
			}
		}
	}

	if (IsValid(WeaponManagerComponent))
	{
		const FPRWeaponVisualInfo& PrimaryVisualInfo = WeaponManagerComponent->GetVisualInfoBySlotType(EPRWeaponSlotType::Primary);
		const FPRWeaponVisualInfo& SecondaryVisualInfo = WeaponManagerComponent->GetVisualInfoBySlotType(EPRWeaponSlotType::Secondary);

		// 현재 무기 데이터는 복제 공개 정보에서 이미 UObject로 보유
		AddUniqueAssetPathFromObject(PrimaryVisualInfo.WeaponData.Get(), OutAssetPaths);
		AddUniqueAssetPathFromObject(PrimaryVisualInfo.ModData.Get(), OutAssetPaths);
		AddUniqueAssetPathFromObject(SecondaryVisualInfo.WeaponData.Get(), OutAssetPaths);
		AddUniqueAssetPathFromObject(SecondaryVisualInfo.ModData.Get(), OutAssetPaths);
	}
}

void UPRAssetUtils::CollectPreviewLoadedRootAssets(const APRPlayerCharacter* SourceCharacter, const UPRWeaponManagerComponent* WeaponManagerComponent, TArray<UObject*>& OutLoadedAssets)
{
	if (IsValid(SourceCharacter))
	{
		if (const UPREquipmentManagerComponent* EquipmentManager = SourceCharacter->GetEquipmentManager())
		{
			for (const FPRReplicatedEquipmentInfo& EquipmentInfo : EquipmentManager->GetEquippedVisualInfos())
			{
				if (UObject* EquipmentData = EquipmentInfo.EquipmentData.Get())
				{
					// 현재 장착 장비 데이터 에셋
					OutLoadedAssets.AddUnique(EquipmentData);
				}
			}
		}
	}

	if (IsValid(WeaponManagerComponent))
	{
		const FPRWeaponVisualInfo& PrimaryVisualInfo = WeaponManagerComponent->GetVisualInfoBySlotType(EPRWeaponSlotType::Primary);
		const FPRWeaponVisualInfo& SecondaryVisualInfo = WeaponManagerComponent->GetVisualInfoBySlotType(EPRWeaponSlotType::Secondary);

		// 현재 무기 공개 비주얼 데이터 에셋
		OutLoadedAssets.AddUnique(PrimaryVisualInfo.WeaponData.Get());
		OutLoadedAssets.AddUnique(PrimaryVisualInfo.ModData.Get());
		OutLoadedAssets.AddUnique(SecondaryVisualInfo.WeaponData.Get());
		OutLoadedAssets.AddUnique(SecondaryVisualInfo.ModData.Get());
	}
}

void UPRAssetUtils::CollectPreviewDependentAssetPaths(const TArray<UObject*>& LoadedAssets, TArray<FSoftObjectPath>& OutAssetPaths)
{
	for (UObject* LoadedAsset : LoadedAssets)
	{
		if (!IsValid(LoadedAsset))
		{
			continue;
		}

		if (const UPREquipmentDataAsset* EquipmentData = Cast<UPREquipmentDataAsset>(LoadedAsset))
		{
			// 장비 외형 메시 경로
			AddUniqueAssetPath(EquipmentData->GetEquipmentMesh().ToSoftObjectPath(), OutAssetPaths);
			continue;
		}

		if (const UPRWeaponDataAsset* WeaponData = Cast<UPRWeaponDataAsset>(LoadedAsset))
		{
			// 무기 액터 클래스 경로
			AddUniqueAssetPathFromObject(WeaponData->WeaponActorClass.Get(), OutAssetPaths);
			continue;
		}

		if (Cast<UPRWeaponModDataAsset>(LoadedAsset))
		{
			// 현재 Mod 데이터 추가 의존 경로 없음
			continue;
		}
	}
}

void UPRAssetUtils::CollectLoadedAssetsFromMap(const TMap<FSoftObjectPath, TWeakObjectPtr<UObject>>& LoadedAssetMap, TArray<UObject*>& OutLoadedAssets)
{
	for (const TPair<FSoftObjectPath, TWeakObjectPtr<UObject>>& LoadedAssetPair : LoadedAssetMap)
	{
		if (UObject* LoadedAsset = LoadedAssetPair.Value.Get())
		{
			// 로드 결과 UObject
			OutLoadedAssets.AddUnique(LoadedAsset);
		}
	}
}

void UPRAssetUtils::AddUniqueAssetPath(const FSoftObjectPath& AssetPath, TArray<FSoftObjectPath>& OutAssetPaths)
{
	if (!AssetPath.IsValid())
	{
		return;
	}

	OutAssetPaths.AddUnique(AssetPath);
}

void UPRAssetUtils::AddUniqueAssetPathFromObject(const UObject* AssetObject, TArray<FSoftObjectPath>& OutAssetPaths)
{
	if (!IsValid(AssetObject))
	{
		return;
	}

	// 이미 로드된 UObject의 패키지 경로
	AddUniqueAssetPath(FSoftObjectPath(AssetObject), OutAssetPaths);
}

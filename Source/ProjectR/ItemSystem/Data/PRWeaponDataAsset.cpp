// Copyright ProjectR. All Rights Reserved.

#include "PRWeaponDataAsset.h"

#include "ProjectR/ItemSystem/Items/PRItemInstance_Weapon.h"
#include "ProjectR/System/PRDeveloperSettings.h"

UPRWeaponDataAsset::UPRWeaponDataAsset()
{
	ItemType = EPRItemType::Weapon;
	ItemInstanceClass = UPRItemInstance_Weapon::StaticClass();
}

const UPRCrosshairConfig* UPRWeaponDataAsset::GetCrosshairConfig() const
{
	if (IsValid(CrosshairConfig))
	{
		return CrosshairConfig;
	}

	const UPRDeveloperSettings* DeveloperSettings = GetDefault<UPRDeveloperSettings>();
	if (!IsValid(DeveloperSettings))
	{
		return nullptr;
	}

	// 무기 전용 설정이 비어 있는 경우 프로젝트 기본 크로스헤어 설정 사용
	return DeveloperSettings->GetDefaultCrosshairConfigSync();
}

void UPRWeaponDataAsset::GiveToAbilitySystem(UAbilitySystemComponent* TargetASC, FPRAbilitySetHandles& OutHandles,
	UObject* InSourceObject)
{
	for (const FPRAbilityEntry& Entry :EquippedAbilities)
	{
		if (!Entry.IsValid())
		{
			continue;
		}

		Entry.GiveToAbilitySystem(TargetASC, OutHandles, InSourceObject);
	}

	for (const FPREffectEntry& Entry :EquippedEffects)
	{
		if (!Entry.IsValid())
		{
			continue;
		}
		
		Entry.GiveToAbilitySystem(TargetASC, OutHandles, InSourceObject);
	}
}

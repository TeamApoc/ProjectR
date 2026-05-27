// Copyright ProjectR. All Rights Reserved.

#include "PRWeaponModDataAsset.h"

#include "ProjectR/ItemSystem/Items/PRItemInstance_Mod.h"

UPRWeaponModDataAsset::UPRWeaponModDataAsset()
{
	ItemType =EPRItemType::Mod;
	ItemInstanceClass = UPRItemInstance_Mod::StaticClass();
}

void UPRWeaponModDataAsset::GiveToAbilitySystem(UAbilitySystemComponent* TargetASC,  FPRAbilitySetHandles& OutHandles, UObject* InSourceObject)
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

// Copyright ProjectR. All Rights Reserved.

#include "PRWeaponDataAsset.h"

#include "ProjectR/ItemSystem/Items/PRItemInstance_Weapon.h"

UPRWeaponDataAsset::UPRWeaponDataAsset()
{
	ItemType = EPRItemType::Weapon;
	ItemInstanceClass = UPRItemInstance_Weapon::StaticClass();
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

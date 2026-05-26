// Copyright ProjectR. All Rights Reserved.

#include "PRWeaponDataAsset.h"

#include "ProjectR/ItemSystem/Items/PRItemInstance_Weapon.h"

UPRWeaponDataAsset::UPRWeaponDataAsset()
{
	SetItemType(EPRItemType::Weapon);
	ItemInstanceClass = UPRItemInstance_Weapon::StaticClass();
}

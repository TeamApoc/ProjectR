// Copyright ProjectR. All Rights Reserved.

#include "PRWeaponModDataAsset.h"

#include "ProjectR/ItemSystem/Items/PRItemInstance_Mod.h"

UPRWeaponModDataAsset::UPRWeaponModDataAsset()
{
	SetItemType(EPRItemType::Mod);
	ItemInstanceClass = UPRItemInstance_Mod::StaticClass();
}

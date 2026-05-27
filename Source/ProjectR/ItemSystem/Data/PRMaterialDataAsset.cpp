// Copyright ProjectR. All Rights Reserved.

#include "PRMaterialDataAsset.h"

#include "ProjectR/ItemSystem/Items/PRItemInstance_Material.h"

UPRMaterialDataAsset::UPRMaterialDataAsset()
{
	ItemType = EPRItemType::Material;
	ItemInstanceClass = UPRItemInstance_Material::StaticClass();
	MaxStackCount = 999;
}

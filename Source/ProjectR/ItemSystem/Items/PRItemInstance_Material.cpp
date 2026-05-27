// Copyright ProjectR. All Rights Reserved.

#include "PRItemInstance_Material.h"

#include "ProjectR/ItemSystem/Data/PRMaterialDataAsset.h"

UPRMaterialDataAsset* UPRItemInstance_Material::GetMaterialData() const
{
	return Cast<UPRMaterialDataAsset>(ItemData);
}

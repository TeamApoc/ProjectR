// Copyright ProjectR. All Rights Reserved.

#include "PREquipmentDataAsset.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Equipment.h"

UPREquipmentDataAsset::UPREquipmentDataAsset()
{
	ItemType = EPRItemType::Equipment;
	ItemInstanceClass = UPRItemInstance_Equipment::StaticClass();
	MaxStackCount = 1; // 장비는 겹쳐지지 않음
}

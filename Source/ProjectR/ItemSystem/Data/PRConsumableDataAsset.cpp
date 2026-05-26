#include "PRConsumableDataAsset.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Consumable.h"

UPRConsumableDataAsset::UPRConsumableDataAsset()
{
	SetItemType(EPRItemType::Consumable);
	ItemInstanceClass = UPRItemInstance_Consumable::StaticClass();
	MaxStackCount = 99;
}

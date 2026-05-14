#include "PRConsumableDataAsset.h"

UPRConsumableDataAsset::UPRConsumableDataAsset()
{
	SetItemType(EPRItemType::Consumable);
	MaxStackCount = 99;
}

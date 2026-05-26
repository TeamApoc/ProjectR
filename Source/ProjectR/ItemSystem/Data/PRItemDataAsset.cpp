#include "PRItemDataAsset.h"

UPRItemDataAsset::UPRItemDataAsset()
{
	ItemType = EPRItemType::None;
}

void UPRItemDataAsset::SetItemType(EPRItemType NewItemType)
{
	ItemType = NewItemType;
}
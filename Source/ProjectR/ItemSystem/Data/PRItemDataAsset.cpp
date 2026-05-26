#include "PRItemDataAsset.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance.h"

UPRItemDataAsset::UPRItemDataAsset()
{
	ItemType = EPRItemType::None;
	ItemInstanceClass = UPRItemInstance::StaticClass();
}

void UPRItemDataAsset::SetItemType(EPRItemType NewItemType)
{
	ItemType = NewItemType;
}

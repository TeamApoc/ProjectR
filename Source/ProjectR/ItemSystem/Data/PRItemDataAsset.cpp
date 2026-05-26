#include "PRItemDataAsset.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance.h"

UPRItemDataAsset::UPRItemDataAsset()
{
	ItemType = EPRItemType::None;
	ItemInstanceClass = UPRItemInstance::StaticClass();
}
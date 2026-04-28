// Copyright ProjectR. All Rights Reserved.

#include "PREquipmentManagerComponent.h"

UPREquipmentManagerComponent::UPREquipmentManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

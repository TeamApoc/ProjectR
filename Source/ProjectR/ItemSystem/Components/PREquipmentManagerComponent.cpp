// Copyright ProjectR. All Rights Reserved.

#include "PREquipmentManagerComponent.h"

UPREquipmentManagerComponent::UPREquipmentManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

FPREquipmentSaveData UPREquipmentManagerComponent::MakeSaveData() const
{
	return FPREquipmentSaveData();
}

void UPREquipmentManagerComponent::ApplySaveData(const FPREquipmentSaveData& InSaveData)
{
	// v1 비무기 장비 복원 대상 없음
}

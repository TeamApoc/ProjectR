// Copyright ProjectR. All Rights Reserved.

#include "PRItemInstance.h"

UPRItemInstance::UPRItemInstance()
{
}

void UPRItemInstance::PostInitProperties()
{
	Super::PostInitProperties();

	// CDO를 제외한 실제 Item 인스턴스만 런타임 식별자를 가진다
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return;
	}

	EnsureItemId();
}

void UPRItemInstance::EnsureItemId()
{
	// 이미 식별자가 있으면 기존 값을 유지한다
	if (ItemId.IsValid())
	{
		return;
	}

	ItemId = FGuid::NewGuid();
}

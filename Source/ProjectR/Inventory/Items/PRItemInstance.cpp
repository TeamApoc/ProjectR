// Copyright ProjectR. All Rights Reserved.

#include "PRItemInstance.h"

UPRItemInstance::UPRItemInstance()
{
}

bool UPRItemInstance::IsSupportedForNetworking() const
{
	return true;
}

void UPRItemInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

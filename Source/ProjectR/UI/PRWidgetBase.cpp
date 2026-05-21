// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#include "PRWidgetBase.h"
#include "ProjectR/System/PREventManagerSubsystem.h"
UPREventManagerSubsystem* UPRWidgetBase::GetEventManager() const
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return nullptr;
	}
	return World->GetSubsystem<UPREventManagerSubsystem>();
}

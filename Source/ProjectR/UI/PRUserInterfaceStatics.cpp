// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRUserInterfaceStatics.h"

#include "ProjectR/System/PRDeveloperSettings.h"

TSubclassOf<UUserWidget> UPRUserInterfaceStatics::GetCrosshairWidgetClass(const EPRCrosshairType CrosshairType)
{
	if (const UPRDeveloperSettings* Settings = GetDefault<UPRDeveloperSettings>())
	{
		return Settings->GetCrosshairWidgetSync(CrosshairType);
	}
	return nullptr;
}

// Copyright ProjectR. All Rights Reserved.

#include "PRDeveloperSettings.h"
#include "Blueprint/UserWidget.h"

TSubclassOf<UUserWidget> UPRDeveloperSettings::GetCrosshairWidgetSync(EPRCrosshairType CrosshairType) const
{
	if (auto Found = CrosshairWidgets.Find(CrosshairType))
	{
		return Found->LoadSynchronous();
	}
	
	return nullptr;
}

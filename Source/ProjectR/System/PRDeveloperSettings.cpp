// Copyright ProjectR. All Rights Reserved.

#include "PRDeveloperSettings.h"
#include "Blueprint/UserWidget.h"
#include "ProjectR/UI/FloatingText/PRFloatingTextWidget.h"

TSubclassOf<UUserWidget> UPRDeveloperSettings::GetCrosshairWidgetSync(EPRCrosshairType CrosshairType) const
{
	if (auto Found = CrosshairWidgets.Find(CrosshairType))
	{
		return Found->LoadSynchronous();
	}
	
	return nullptr;
}

FPRFloatingTextStyle UPRDeveloperSettings::GetFloatingTextStyleSync(EPRFloatingTextType TextType) const
{
	if (const FPRFloatingTextStyleConfig* Found = FloatingTextStyles.Find(TextType))
	{
		FPRFloatingTextStyle Result;
		Result.WidgetClass = Found->WidgetClass.LoadSynchronous();
		Result.Color = Found->Color;
		return Result;
	}

	return FPRFloatingTextStyle();
}

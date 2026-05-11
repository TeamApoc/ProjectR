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

FText UPRUserInterfaceStatics::ConvertFloatToText(float Value, int32 MaximumFractionalDigits)
{
	FNumberFormattingOptions FormattingOptions;
	FormattingOptions.MaximumFractionalDigits = FMath::Max(MaximumFractionalDigits, 0);
	FormattingOptions.MinimumFractionalDigits = 0;
	return FText::AsNumber(Value, &FormattingOptions);
}

float UPRUserInterfaceStatics::ConvertMinMaxToPercent(float CurrentValue, float MaxValue, float MinValue)
{
	if (MaxValue <= MinValue)
	{
		return 0.0f;
	}

	return FMath::Clamp((CurrentValue - MinValue) / (MaxValue - MinValue), 0.0f, 1.0f);
}

// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PRUserInterfaceStatics.generated.h"

enum class EPRCrosshairType : uint8;
/**
 * 
 */
UCLASS()
class PROJECTR_API UPRUserInterfaceStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable)
	static TSubclassOf<UUserWidget> GetCrosshairWidgetClass(const EPRCrosshairType CrosshairType);
};

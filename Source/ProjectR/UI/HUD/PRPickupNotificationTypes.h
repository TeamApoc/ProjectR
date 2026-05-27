// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PRPickupNotificationTypes.generated.h"

class UTexture2D;

USTRUCT(BlueprintType)
struct FPRPickupNotificationViewData
{
	GENERATED_BODY()

	// 알림에 표시할 문구
	UPROPERTY(BlueprintReadOnly)
	FText InfoText;

	// 알림에 표시할 아이콘
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UTexture2D> Icon = nullptr;
};

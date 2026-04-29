// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PRWeaponAnimationTypes.generated.h"

UENUM(BlueprintType)
enum class EPRWeaponAnimationState : uint8
{
	Idle,
	Shoot,
	Reload
};

// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PRItemTypes.generated.h"

UENUM(BlueprintType)
enum class EPRItemType : uint8
{
	None,
	Weapon,
	Mod,
	PrimaryWeapon,
	SecondaryWeapon,
	Consumable,
	Material,
	Equipment
};

UENUM(BlueprintType)
enum class EPRItemRarity : uint8
{
	Common,
	Rare,
	Epic,
	Legendary
};

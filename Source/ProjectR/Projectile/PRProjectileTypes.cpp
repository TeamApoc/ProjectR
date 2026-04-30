// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#include "PRProjectileTypes.h"

bool FPRProjectileRepMovement::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	uint8 EventByte = static_cast<uint8>(Event);
	Ar.SerializeBits(&EventByte, 2);
	Event = static_cast<EPRRepMovementEvent>(EventByte);

	Location.NetSerialize(Ar, Map, bOutSuccess);
	Velocity.NetSerialize(Ar, Map, bOutSuccess);
	Rotation.NetSerialize(Ar, Map, bOutSuccess);

	bOutSuccess = true;
	return true;
}

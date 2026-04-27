// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRProjectileTypes.h"

bool FPRTargetData_SpawnProjectile::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	Ar << ProjectileId;
	SpawnLocation.NetSerialize(Ar, Map, bOutSuccess);
	SpawnRotation.NetSerialize(Ar, Map, bOutSuccess);
	bOutSuccess = true;
	return true;
}

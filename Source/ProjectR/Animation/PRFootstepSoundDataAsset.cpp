// Copyright ProjectR. All Rights Reserved.

#include "PRFootstepSoundDataAsset.h"

FName UPRFootstepSoundDataAsset::ResolveFootSocketName(EPRFootstepFoot Foot) const
{
	switch (Foot)
	{
	case EPRFootstepFoot::Left:
		return LeftFootSocketName;

	case EPRFootstepFoot::Right:
		return RightFootSocketName;

	default:
		return NAME_None;
	}
}

const FPRFootstepSoundEntry* UPRFootstepSoundDataAsset::FindSoundEntry(EPhysicalSurface SurfaceType) const
{
	const TEnumAsByte<EPhysicalSurface> SurfaceKey(SurfaceType);
	if (const FPRFootstepSoundEntry* FoundEntry = SurfaceEntries.Find(SurfaceKey))
	{
		return FoundEntry;
	}

	return &DefaultEntry;
}

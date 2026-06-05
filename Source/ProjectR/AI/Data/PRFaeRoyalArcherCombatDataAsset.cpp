// Copyright ProjectR. All Rights Reserved.

#include "PRFaeRoyalArcherCombatDataAsset.h"

UPRFaeRoyalArcherCombatDataAsset::UPRFaeRoyalArcherCombatDataAsset()
{
	AirCombatPresentationConfig.bMaintainTargetFocus = true;
	AirCombatPresentationConfig.bUseCombatMovePose = true;
	AirCombatPresentationConfig.bUseCombatAimOffset = true;
	AirCombatPresentationConfig.bUseCombatStrafeState = true;
	AirCombatPresentationConfig.bUseControllerDesiredRotation = true;
	AirCombatPresentationConfig.bOrientRotationToMovement = false;
	AirCombatPresentationConfig.MoveSpeedOverride = 0.0f;
	AirCombatPresentationConfig.RotationYawRate = 540.0f;
}

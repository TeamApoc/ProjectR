// Copyright ProjectR. All Rights Reserved.

#include "PRFaeRoyalArcherCombatDataAsset.h"

UPRFaeRoyalArcherCombatDataAsset::UPRFaeRoyalArcherCombatDataAsset()
{
	AirCombatPositionQueryConfig.CandidateSelectionMode = EPREnemyQueryCandidateSelectionMode::WeightedRandomTopCandidates;
	AirCombatPositionQueryConfig.TopCandidateCount = 5;
	AirCombatPositionQueryConfig.TopScoreCandidateRatio = 0.85f;
	AirPositionDistanceOffsets = { -250.0f, 0.0f, 250.0f };
	AirPositionHeightOffsets = { -120.0f, 0.0f, 180.0f };

	AirCombatPresentationConfig.bMaintainTargetFocus = true;
	AirCombatPresentationConfig.bUseCombatMovePose = true;
	AirCombatPresentationConfig.bUseCombatAimOffset = true;
	AirCombatPresentationConfig.bUseCombatStrafeState = true;
	AirCombatPresentationConfig.bUseControllerDesiredRotation = true;
	AirCombatPresentationConfig.bOrientRotationToMovement = false;
	AirCombatPresentationConfig.MoveSpeedOverride = 0.0f;
	AirCombatPresentationConfig.RotationYawRate = 540.0f;
}

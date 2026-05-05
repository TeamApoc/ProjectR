// Copyright ProjectR. All Rights Reserved.

#include "PRFaerinCharacter.h"

APRFaerinCharacter::APRFaerinCharacter()
{
	PhaseThresholdRatios.Add(EPRBossPhase::Phase2, 0.87f);
	PhaseThresholdRatios.Add(EPRBossPhase::Phase3, 0.65f);
	PhaseThresholdRatios.Add(EPRBossPhase::Phase4, 0.25f);
}

// Copyright ProjectR. All Rights Reserved.

#include "PRBGMRegistryDataAsset.h"

bool UPRBGMRegistryDataAsset::FindLevelEntry(FName LevelId, FPRLevelBGMEntry& OutEntry) const
{
	if (LevelId.IsNone())
	{
		return false;
	}

	for (const FPRLevelBGMEntry& Entry : LevelEntries)
	{
		if (Entry.LevelId == LevelId)
		{
			OutEntry = Entry;
			return true;
		}
	}

	return false;
}

bool UPRBGMRegistryDataAsset::TryGetTrackForState(const FPRLevelBGMEntry& Entry, EPRBGMState State, FPRBGMTrack& OutTrack)
{
	switch (State)
	{
	case EPRBGMState::Hub:
		OutTrack = Entry.HubTrack;
		return true;
	case EPRBGMState::Exploration:
		OutTrack = Entry.ExplorationTrack;
		return true;
	case EPRBGMState::Combat:
		OutTrack = Entry.CombatTrack;
		return true;
	case EPRBGMState::BossPhaseA:
		OutTrack = Entry.BossPhaseATrack;
		return true;
	case EPRBGMState::BossPhaseB:
		OutTrack = Entry.BossPhaseBTrack;
		return true;
	case EPRBGMState::BossPhaseC:
		OutTrack = Entry.BossPhaseCTrack;
		return true;
	case EPRBGMState::BossPhaseD:
		OutTrack = Entry.BossPhaseDTrack;
		return true;
	case EPRBGMState::Victory:
		OutTrack = Entry.VictoryTrack;
		return true;
	case EPRBGMState::None:
	default:
		return false;
	}
}

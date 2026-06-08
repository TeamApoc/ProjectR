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
	case EPRBGMState::BossPhase1:
		OutTrack = Entry.BossPhase1Track;
		return true;
	case EPRBGMState::BossPhase2:
		OutTrack = Entry.BossPhase2Track;
		return true;
	case EPRBGMState::BossPhase3:
		OutTrack = Entry.BossPhase3Track;
		return true;
	case EPRBGMState::BossPhase4:
		OutTrack = Entry.BossPhase4Track;
		return true;
	case EPRBGMState::Victory:
		OutTrack = Entry.VictoryTrack;
		return true;
	case EPRBGMState::None:
	default:
		return false;
	}
}

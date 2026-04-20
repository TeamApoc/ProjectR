// Copyright ProjectR. All Rights Reserved.

#include "PRGameStateBase.h"
#include "Net/UnrealNetwork.h"

// ===== 복제 등록 =====

void APRGameStateBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APRGameStateBase, ActiveCheckpoint);
	DOREPLIFETIME(APRGameStateBase, UnlockedCheckpoints);
	DOREPLIFETIME(APRGameStateBase, DefeatedBosses);
	DOREPLIFETIME(APRGameStateBase, WorldSaveVersion);
}

// ===== 조회 =====

bool APRGameStateBase::IsBossDefeated(FName BossId) const
{
	return DefeatedBosses.Contains(BossId);
}

// ===== 서버 권위 변경 =====

void APRGameStateBase::InitializeFromWorldSave(const FPRWorldSaveData& WorldSave)
{
	if (!HasAuthority())
	{
		return;
	}

	WorldSaveVersion     = WorldSave.Version;
	ActiveCheckpoint     = WorldSave.LastCheckpointId;
	UnlockedCheckpoints  = WorldSave.UnlockedCheckpoints;
	DefeatedBosses       = WorldSave.DefeatedBosses;
}

void APRGameStateBase::SetActiveCheckpoint(FName CheckpointId)
{
	if (!HasAuthority())
	{
		return;
	}

	ActiveCheckpoint = CheckpointId;
	UnlockedCheckpoints.AddUnique(CheckpointId);

	// 서버 로컬에서도 이벤트 발행(호스트 UI 갱신)
	OnCheckpointActivated.Broadcast(CheckpointId);
}

void APRGameStateBase::MarkBossDefeated(FName BossId)
{
	if (!HasAuthority())
	{
		return;
	}

	if (DefeatedBosses.Contains(BossId))
	{
		return;
	}

	DefeatedBosses.Add(BossId);
	OnBossDefeated.Broadcast(BossId);
}

// ===== 복제 콜백 =====

void APRGameStateBase::OnRep_ActiveCheckpoint(FName OldCheckpoint)
{
	if (ActiveCheckpoint == OldCheckpoint)
	{
		return;
	}

	OnCheckpointActivated.Broadcast(ActiveCheckpoint);
}

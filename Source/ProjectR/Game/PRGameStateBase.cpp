// Copyright ProjectR. All Rights Reserved.

#include "PRGameStateBase.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/PlayerState.h"
#include "ProjectR/Character/PRPlayerCharacter.h"


void APRGameStateBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APRGameStateBase, ActiveCheckpoint);
	DOREPLIFETIME(APRGameStateBase, UnlockedCheckpoints);
	DOREPLIFETIME(APRGameStateBase, LastActiveWaypointId);
	DOREPLIFETIME(APRGameStateBase, DefeatedBosses);
	DOREPLIFETIME(APRGameStateBase, WorldSaveVersion);
}

bool APRGameStateBase::IsBossDefeated(FName BossId) const
{
	return DefeatedBosses.Contains(BossId);
}

TArray<APRPlayerCharacter*> APRGameStateBase::GetPlayerCharacters() const
{
	TArray<APRPlayerCharacter*> OutCharacters;
	OutCharacters.Reserve(PlayerArray.Num());

	for (APlayerState* PS : PlayerArray)
	{
		if (!IsValid(PS))
		{
			continue;
		}

		APRPlayerCharacter* PlayerCharacter = Cast<APRPlayerCharacter>(PS->GetPawn());
		if (!IsValid(PlayerCharacter))
		{
			continue;
		}

		OutCharacters.Add(PlayerCharacter);
	}

	return OutCharacters;
}

void APRGameStateBase::InitializeFromWorldSave(const FPRWorldSaveData& WorldSave)
{
	if (!HasAuthority())
	{
		return;
	}

	WorldSaveVersion     = WorldSave.Version;
	ActiveCheckpoint     = WorldSave.LastCheckpointId;
	LastActiveWaypointId = WorldSave.LastActiveWaypointId;
	UnlockedCheckpoints  = WorldSave.UnlockedCheckpoints;
	DefeatedBosses       = WorldSave.DefeatedBosses;
}

FPRWorldSaveData APRGameStateBase::MakeWorldSaveData() const
{
	FPRWorldSaveData SaveData;
	SaveData.Version = WorldSaveVersion;
	SaveData.LastCheckpointId = ActiveCheckpoint;
	SaveData.LastActiveWaypointId = LastActiveWaypointId;
	SaveData.UnlockedCheckpoints = UnlockedCheckpoints;
	SaveData.DefeatedBosses = DefeatedBosses;
	return SaveData;
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

void APRGameStateBase::SetLastActiveWaypointId(FGameplayTag WaypointId)
{
	// 서버에서만 상태 변경
	if (!HasAuthority())
	{
		return;
	}

	LastActiveWaypointId = WaypointId;
	UE_LOG(LogTemp, Log, TEXT("LastActiveWaypointId updated: %s"), *LastActiveWaypointId.ToString());
}

void APRGameStateBase::ClearLastActiveWaypointId()
{
	// 서버에서만 상태 초기화
	if (!HasAuthority())
	{
		return;
	}

	LastActiveWaypointId = FGameplayTag();
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

void APRGameStateBase::OnRep_ActiveCheckpoint(FName OldCheckpoint)
{
	if (ActiveCheckpoint == OldCheckpoint)
	{
		return;
	}

	OnCheckpointActivated.Broadcast(ActiveCheckpoint);
}

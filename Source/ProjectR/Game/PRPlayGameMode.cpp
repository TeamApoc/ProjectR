// Copyright ProjectR. All Rights Reserved.

#include "PRPlayGameMode.h"
#include "PRGameStateBase.h"
#include "ProjectR/Player/PRPlayerController.h"
#include "ProjectR/Player/PRPlayerState.h"

APRPlayGameMode::APRPlayGameMode()
{
	PlayerControllerClass = APRPlayerController::StaticClass();
	PlayerStateClass      = APRPlayerState::StaticClass();
	GameStateClass        = APRGameStateBase::StaticClass();

	bUseSeamlessTravel = true;
}

void APRPlayGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	// 월드 세이브 로드는 세이브 시스템 구현 시 여기서 수행
	// 현재는 기본 초기값을 유지한다
	HostWorldSave.Version = EPRSaveVersion::V1;
}

void APRPlayGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	// GameState에 월드 세이브 주입 (최초 1회만 수행)
	if (APRGameStateBase* GS = GetGameState<APRGameStateBase>())
	{
		if (GS->GetActiveCheckpoint().IsNone() && !HostWorldSave.LastCheckpointId.IsNone())
		{
			GS->InitializeFromWorldSave(HostWorldSave);
		}
	}

	// 호스트 PlayerController는 자체적으로 캐릭터 데이터를 들고 있으므로
	// 클라이언트 페이로드 수신은 ServerSubmitCharacter RPC 경로를 통해 비동기로 들어온다
}

void APRPlayGameMode::Logout(AController* Exiting)
{
	// 게스트 이탈 시 누적 보상을 정산하여 ClientCommitRewards로 전달
	if (APRPlayerController* Guest = Cast<APRPlayerController>(Exiting))
	{
		const FPRGuestRewardBatch Batch = CollectRewardsForGuest(Guest);
		Guest->ClientCommitRewards(Batch);
	}

	Super::Logout(Exiting);
}

void APRPlayGameMode::ReportCheckpointActivated(FName CheckpointId)
{
	if (APRGameStateBase* GS = GetGameState<APRGameStateBase>())
	{
		GS->SetActiveCheckpoint(CheckpointId);
	}
}

void APRPlayGameMode::ReportBossDefeated(FName BossId)
{
	if (APRGameStateBase* GS = GetGameState<APRGameStateBase>())
	{
		GS->MarkBossDefeated(BossId);
	}
}

bool APRPlayGameMode::ValidateCharacterPayload(const FPRCharacterSaveData& Payload, FString& OutReason) const
{
	// 포맷 버전 체크
	if (Payload.Version != EPRSaveVersion::V1)
	{
		OutReason = TEXT("Save version mismatch");
		return false;
	}

	// 표시명 길이 체크
	if (Payload.DisplayName.Len() == 0 || Payload.DisplayName.Len() > MaxDisplayNameLength)
	{
		OutReason = TEXT("Invalid display name length");
		return false;
	}

	// 레벨 상한 체크
	if (Payload.Level < 1 || Payload.Level > MaxCharacterLevel)
	{
		OutReason = TEXT("Level out of range");
		return false;
	}

	// 경험치 음수 방지
	if (Payload.Experience < 0)
	{
		OutReason = TEXT("Negative experience");
		return false;
	}

	// 스탯 음수 방지
	if (Payload.Stats.BaseMaxHealth <= 0.f || Payload.Stats.BaseMaxStamina <= 0.f)
	{
		OutReason = TEXT("Non-positive base stats");
		return false;
	}

	return true;
}

bool APRPlayGameMode::AcceptGuestCharacter(APRPlayerController* From, const FPRCharacterSaveData& Payload)
{
	if (!IsValid(From))
	{
		return false;
	}

	FString Reason;
	if (!ValidateCharacterPayload(Payload, Reason))
	{
		return false;
	}

	// PlayerState에 주입. 이후 복제로 모든 클라에 전파
	if (APRPlayerState* PS = From->GetPlayerState<APRPlayerState>())
	{
		PS->InitializeFromSaveData(Payload);
		return true;
	}

	return false;
}

FPRGuestRewardBatch APRPlayGameMode::CollectRewardsForGuest(APRPlayerController* Guest) const
{
	// 보상 시스템 구현 시 PlayerState의 누적 값을 배치로 변환
	// 현재는 빈 배치 반환 (스텁)
	return FPRGuestRewardBatch();
}

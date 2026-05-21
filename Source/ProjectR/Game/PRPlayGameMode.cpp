// Copyright ProjectR. All Rights Reserved.

#include "PRPlayGameMode.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerStart.h"
#include "PRGameStateBase.h"
#include "TimerManager.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/Game/PRGameInstance.h"
#include "ProjectR/Player/PRPlayerController.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "ProjectR/World/PRWaypointActor.h"
#include "ProjectR/World/PRWaypointTags.h"

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

	if (UPRGameInstance* GameInstance = GetGameInstance<UPRGameInstance>())
	{
		// ServerTravel 진입 Waypoint 소비
		TravelSpawnWaypointId = GameInstance->ConsumePendingTravelWaypointId();
	}
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

		if (TravelSpawnWaypointId.IsValid() && !GS->GetLastActiveWaypointId().IsValid())
		{
			// 맵 이동 진입 Waypoint 활성화
			GS->SetLastActiveWaypointId(TravelSpawnWaypointId);
		}
	}

	// 호스트 PlayerController는 자체적으로 캐릭터 데이터를 들고 있으므로
	// 클라이언트 페이로드 수신은 ServerSubmitCharacter RPC 경로를 통해 비동기로 들어온다
}

void APRPlayGameMode::Logout(AController* Exiting)
{
	// 보상은 즉시 지급 원칙이므로 여기서 Reliable RPC를 호출하지 않는다
	// (연결이 이미 해제 단계이므로 도달이 보장되지 않음)
	// 필요한 정리 작업(월드 오브젝트 소유권 이관 등)만 수행한다
	Super::Logout(Exiting);
}

AActor* APRPlayGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	// Waypoint 우선 스폰 지점
	const FGameplayTag SpawnWaypointId = ResolvePlayerStartWaypointId();
	if (APRWaypointActor* Waypoint = FindWaypointById(SpawnWaypointId))
	{
		const int32 PlayerIndex = ResolvePlayerIndex(Player);
		if (APlayerStart* LinkedPlayerStart = Waypoint->GetLinkedPlayerStart(PlayerIndex))
		{
			return LinkedPlayerStart;
		}

		return Waypoint;
	}

	return Super::ChoosePlayerStart_Implementation(Player);
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
		PS->InitializePrimaryInfoFromSaveData(Payload);
		return true;
	}

	return false;
}

void APRPlayGameMode::GrantRewardTo(APRPlayerController* Target, const FPRRewardGrant& Grant)
{
	if (!IsValid(Target))
	{
		return;
	}

	// 오너 클라에게만 푸시. 수신 측에서 GameInstance에 즉시 반영
	Target->ClientGrantReward(Grant);
}

void APRPlayGameMode::NotifyPlayerSurvivalStateChanged(APRPlayerState* PlayerState)
{
	if (!HasAuthority() || !IsValid(PlayerState) || !PlayerState->IsCombatParticipant())
	{
		return;
	}

	EvaluatePartyWipe();
}

void APRPlayGameMode::EvaluatePartyWipe()
{
	if (!HasAuthority() || bPartyWipeInProgress)
	{
		return;
	}

	const AGameStateBase* CurrentGameState = GameState;
	if (!IsValid(CurrentGameState))
	{
		return;
	}

	int32 ParticipantCount = 0;
	int32 OutOfFightCount = 0;
	for (APlayerState* PlayerState : CurrentGameState->PlayerArray)
	{
		const APRPlayerState* PRPlayerState = Cast<APRPlayerState>(PlayerState);
		if (!IsValid(PRPlayerState) || !PRPlayerState->IsCombatParticipant())
		{
			continue;
		}

		++ParticipantCount;
		if (PRPlayerState->IsOutOfFight())
		{
			++OutOfFightCount;
		}
	}

	if (ParticipantCount > 0 && ParticipantCount == OutOfFightCount)
	{
		ConfirmPartyWipe();
	}
}

void APRPlayGameMode::ConfirmPartyWipe()
{
	if (!HasAuthority() || bPartyWipeInProgress)
	{
		return;
	}

	AGameStateBase* CurrentGameState = GameState;
	if (!IsValid(CurrentGameState))
	{
		return;
	}

	bPartyWipeInProgress = true;
	for (APlayerState* PlayerState : CurrentGameState->PlayerArray)
	{
		APRPlayerState* PRPlayerState = Cast<APRPlayerState>(PlayerState);
		if (!IsValid(PRPlayerState) || !PRPlayerState->IsCombatParticipant())
		{
			continue;
		}

		if (!PRPlayerState->IsDead())
		{
			PRPlayerState->SendSurvivalGameplayEvent(PRGameplayTags::Event_Ability_Death);
		}

		APRPlayerController* PlayerController = Cast<APRPlayerController>(PRPlayerState->GetOwner());
		if (IsValid(PlayerController))
		{
			// 리스폰 전환 연출
			PlayerController->ClientStartMapTransition(PartyWipeRespawnDelay, EPRMapTransitionType::Respawn);
		}
		
		// if (PRPlayerState->IsDown())
		// {
		// 	PRPlayerState->SendSurvivalGameplayEvent(PRGameplayTags::Event_Ability_PlayerDeathConfirmed);
		// }
		// else if (!PRPlayerState->IsDead())
		// {
		// 	PRPlayerState->SendSurvivalGameplayEvent(PRGameplayTags::Event_Ability_Death);
		// }
	}

	if (UWorld* World = GetWorld())
	{
		// 사망 연출 대기
		World->GetTimerManager().SetTimer(
			PartyWipeRespawnTimerHandle,
			this,
			&ThisClass::RespawnPartyAtLastActiveWaypoint,
			PartyWipeRespawnDelay,
			false);
	}
	else
	{
		RespawnPartyAtLastActiveWaypoint();
	}
}

void APRPlayGameMode::RespawnPartyAtLastActiveWaypoint()
{
	// 서버 권위 리스폰
	if (!HasAuthority())
	{
		return;
	}

	AGameStateBase* CurrentGameState = GameState;
	if (!IsValid(CurrentGameState))
	{
		bPartyWipeInProgress = false;
		return;
	}

	const FGameplayTag RespawnWaypointId = ResolvePartyRespawnWaypointId();
	const APRWaypointActor* RespawnWaypoint = FindWaypointById(RespawnWaypointId);
	const bool bUseWaypointTransform = IsValid(RespawnWaypoint);

	for (APlayerState* PlayerState : CurrentGameState->PlayerArray)
	{
		APRPlayerState* PRPlayerState = Cast<APRPlayerState>(PlayerState);
		if (!IsValid(PRPlayerState) || !PRPlayerState->IsCombatParticipant())
		{
			continue;
		}

		AController* Controller = Cast<AController>(PRPlayerState->GetOwner());
		if (!IsValid(Controller) && IsValid(PRPlayerState->GetPawn()))
		{
			Controller = PRPlayerState->GetPawn()->GetController();
		}

		if (!IsValid(Controller))
		{
			continue;
		}

		// ASC 생존 상태 초기화
		PRPlayerState->ResetSurvivalStateForRespawn();

		if (APawn* OldPawn = Controller->GetPawn())
		{
			// 기존 Pawn 정리
			Controller->UnPossess();
			OldPawn->Destroy();
		}

		if (bUseWaypointTransform)
		{
			// 플레이어별 Waypoint 스폰 위치
			const FTransform RespawnTransform = RespawnWaypoint->GetSpawnTransform(ResolvePlayerIndex(Controller));
			RestartPlayerAtTransform(Controller, RespawnTransform);
		}
		else
		{
			RestartPlayer(Controller);
		}
	}

	bPartyWipeInProgress = false;
}

APRWaypointActor* APRPlayGameMode::FindWaypointById(FGameplayTag WaypointId) const
{
	// 현재 월드 탐색
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return nullptr;
	}

	const FGameplayTag TargetWaypointId = WaypointId.IsValid() ? WaypointId : PRWaypointTags::Waypoint_Default;
	APRWaypointActor* FoundWaypoint = nullptr;

	for (TActorIterator<APRWaypointActor> It(World); It; ++It)
	{
		APRWaypointActor* Waypoint = *It;
		if (!IsValid(Waypoint) || !Waypoint->MatchesWaypointId(TargetWaypointId))
		{
			continue;
		}

		if (IsValid(FoundWaypoint))
		{
			// 중복 Waypoint 진단
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("Duplicate waypoint tag %s found. Using first waypoint %s."),
				*TargetWaypointId.ToString(),
				*FoundWaypoint->GetName());
			continue;
		}

		FoundWaypoint = Waypoint;
	}

	return FoundWaypoint;
}

FGameplayTag APRPlayGameMode::ResolvePlayerStartWaypointId() const
{
	const APRGameStateBase* PRGameState = Cast<APRGameStateBase>(GameState);

	// 맵 이동 진입 지점 우선
	if (TravelSpawnWaypointId.IsValid()
		&& (!IsValid(PRGameState) || !PRGameState->GetLastActiveWaypointId().IsValid()))
	{
		return TravelSpawnWaypointId;
	}

	if (IsValid(PRGameState) && PRGameState->GetLastActiveWaypointId().IsValid())
	{
		return PRGameState->GetLastActiveWaypointId();
	}

	return PRWaypointTags::Waypoint_Default;
}

FGameplayTag APRPlayGameMode::ResolvePartyRespawnWaypointId() const
{
	// 전멸 리스폰 지점 우선
	const APRGameStateBase* PRGameState = Cast<APRGameStateBase>(GameState);
	if (IsValid(PRGameState) && PRGameState->GetLastActiveWaypointId().IsValid())
	{
		const FGameplayTag RespawnWaypointId = PRGameState->GetLastActiveWaypointId();
		UE_LOG(LogTemp, Log, TEXT("Party respawn waypoint resolved from LastActiveWaypointId: %s"), *RespawnWaypointId.ToString());
		return RespawnWaypointId;
	}

	UE_LOG(LogTemp, Log, TEXT("Party respawn waypoint fallback to default"));
	return PRWaypointTags::Waypoint_Default;
}

int32 APRPlayGameMode::ResolvePlayerIndex(const AController* Controller) const
{
	// 컨트롤러 미지정
	if (!IsValid(Controller))
	{
		return INDEX_NONE;
	}

	const AGameStateBase* CurrentGameState = GameState;
	if (!IsValid(CurrentGameState))
	{
		return INDEX_NONE;
	}

	const APlayerState* TargetPlayerState = Controller->PlayerState;
	for (int32 PlayerIndex = 0; PlayerIndex < CurrentGameState->PlayerArray.Num(); ++PlayerIndex)
	{
		if (CurrentGameState->PlayerArray[PlayerIndex] == TargetPlayerState)
		{
			// PlayerArray 순번
			return PlayerIndex;
		}
	}

	return INDEX_NONE;
}

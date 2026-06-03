// Copyright ProjectR. All Rights Reserved.

#include "PRRespawnSubsystem.h"

#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerStart.h"
#include "ProjectR/Player/PRPlayerController.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "ProjectR/World/PRSpawnPoint.h"
#include "ProjectR/World/PRSpawnPointTags.h"

/*~ 등록 ~*/

void UPRRespawnSubsystem::RegisterRespawnableActor(AActor* InActor)
{
	// 등록 대상 검증
	if (!IsValid(InActor) || !InActor->HasAuthority())
	{
		return;
	}

	if (InActor->GetWorld() != GetWorld())
	{
		// 다른 월드 액터 등록 방지
		return;
	}

	for (const FPRRespawnActorInfo& RegisteredInfo : RegisteredActors)
	{
		if (RegisteredInfo.Actor.Get() == InActor)
		{
			// 동일 액터 중복 등록 방지
			return;
		}
	}

	FPRRespawnActorInfo NewInfo;
	NewInfo.Actor = InActor;
	NewInfo.ActorClass = InActor->GetClass();
	NewInfo.SpawnTransform = InActor->GetActorTransform();
	RegisteredActors.Add(NewInfo);
}

void UPRRespawnSubsystem::RegisterDisposableActor(AActor* InActor)
{
	// 등록 대상 검증
	if (!IsValid(InActor) || !InActor->HasAuthority())
	{
		return;
	}

	if (InActor->GetWorld() != GetWorld())
	{
		// 다른 월드 액터 등록 방지
		return;
	}

	TWeakObjectPtr<AActor> DisposableActorPtr = InActor;

	// 동일 액터 중복 등록 방지
	DisposableActors.Add(DisposableActorPtr);
}

void UPRRespawnSubsystem::UnregisterDisposableActor(AActor* InActor)
{
	if (!IsValid(InActor))
	{
		return;
	}

	// 일회성 액터 제거 대상 해제
	TWeakObjectPtr<AActor> DisposableActorPtr = InActor;
	DisposableActors.Remove(DisposableActorPtr);
}

/*~ 리스폰 실행 ~*/

bool UPRRespawnSubsystem::RespawnWorldObjects()
{
	if (!IsServerWorld())
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return false;
	}

	// 등록 목록 격리
	TArray<FPRRespawnActorInfo> RespawnInfos = RegisteredActors;
	TSet<TWeakObjectPtr<AActor>> DisposableInfos = DisposableActors;
	RegisteredActors.Empty();
	DisposableActors.Empty();

	bool bProcessedAny = false;
	for (const TWeakObjectPtr<AActor>& DisposableActorPtr : DisposableInfos)
	{
		AActor* DisposableActor = DisposableActorPtr.Get();
		if (!IsValid(DisposableActor))
		{
			continue;
		}

		// 일회성 액터 파괴
		DisposableActor->Destroy();
		bProcessedAny = true;
	}

	for (const FPRRespawnActorInfo& RespawnInfo : RespawnInfos)
	{
		if (AActor* ExistingActor = RespawnInfo.Actor.Get())
		{
			// 기존 액터 파괴
			ExistingActor->Destroy();
			bProcessedAny = true;
		}

		if (!IsValid(RespawnInfo.ActorClass.Get()))
		{
			// 생성 클래스 없음
			continue;
		}

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		// 등록 Transform 기준 재생성
		AActor* SpawnedActor = World->SpawnActor<AActor>(
			RespawnInfo.ActorClass,
			RespawnInfo.SpawnTransform,
			SpawnParameters);

		bProcessedAny |= IsValid(SpawnedActor);
	}

	return bProcessedAny;
}

bool UPRRespawnSubsystem::RespawnPlayers(FGameplayTag SpawnPointId)
{
	if (!IsServerWorld())
	{
		return false;
	}

	UWorld* World = GetWorld();
	AGameModeBase* GameMode = IsValid(World) ? World->GetAuthGameMode() : nullptr;
	AGameStateBase* GameState = IsValid(World) ? World->GetGameState() : nullptr;
	if (!IsValid(World) || !IsValid(GameMode) || !IsValid(GameState))
	{
		return false;
	}

	bool bProcessedAny = false;
	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		APRPlayerState* PRPlayerState = Cast<APRPlayerState>(PlayerState);
		if (!IsValid(PRPlayerState) || !PRPlayerState->IsCombatParticipant())
		{
			continue;
		}

		AController* Controller = ResolveControllerFromPlayerState(PRPlayerState);
		if (!IsValid(Controller))
		{
			continue;
		}

		// PlayerState 런타임 상태 정리
		PRPlayerState->ResetState();

		// 기존 Pawn 제거
		DestroyControllerPawn(Controller);

		FTransform RespawnTransform;
		if (ResolvePlayerRespawnTransform(Controller, SpawnPointId, RespawnTransform))
		{
			// 체크포인트 위치 재시작
			GameMode->RestartPlayerAtTransform(Controller, RespawnTransform);
		}
		else
		{
			// 기본 PlayerStart 재시작
			GameMode->RestartPlayer(Controller);
		}

		NotifyRespawnComplete(Controller);
		bProcessedAny = true;
	}

	return bProcessedAny;
}

bool UPRRespawnSubsystem::RespawnWorldAndPlayers(FGameplayTag SpawnPointId)
{
	const bool bWorldRespawned = RespawnWorldObjects();
	const bool bPlayersRespawned = RespawnPlayers(SpawnPointId);
	return bWorldRespawned || bPlayersRespawned;
}

void UPRRespawnSubsystem::ClearRegisteredActors()
{
	RegisteredActors.Empty();
	DisposableActors.Empty();
}

/*~ SpawnPoint 조회 ~*/

APRSpawnPoint* UPRRespawnSubsystem::FindSpawnPointById(FGameplayTag SpawnPointId) const
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return nullptr;
	}

	const FGameplayTag TargetSpawnPointId = SpawnPointId.IsValid() ? SpawnPointId : PRSpawnPointTags::SpawnPoint_Default;
	APRSpawnPoint* FoundSpawnPoint = nullptr;

	for (TActorIterator<APRSpawnPoint> It(World); It; ++It)
	{
		APRSpawnPoint* SpawnPoint = *It;
		if (!IsValid(SpawnPoint) || !SpawnPoint->MatchesSpawnPointId(TargetSpawnPointId))
		{
			continue;
		}

		if (IsValid(FoundSpawnPoint))
		{
			// 중복 SpawnPoint 진단
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("Duplicate spawn point tag %s found. Using first spawn point %s."),
				*TargetSpawnPointId.ToString(),
				*FoundSpawnPoint->GetName());
			continue;
		}

		FoundSpawnPoint = SpawnPoint;
	}

	return FoundSpawnPoint;
}

int32 UPRRespawnSubsystem::ResolvePlayerIndex(const AController* Controller) const
{
	if (!IsValid(Controller))
	{
		return INDEX_NONE;
	}

	UWorld* World = GetWorld();
	const AGameStateBase* GameState = IsValid(World) ? World->GetGameState() : nullptr;
	if (!IsValid(GameState))
	{
		return INDEX_NONE;
	}

	const APlayerState* TargetPlayerState = Controller->PlayerState;
	for (int32 PlayerIndex = 0; PlayerIndex < GameState->PlayerArray.Num(); ++PlayerIndex)
	{
		if (GameState->PlayerArray[PlayerIndex] == TargetPlayerState)
		{
			// PlayerArray 순번
			return PlayerIndex;
		}
	}

	return INDEX_NONE;
}

AActor* UPRRespawnSubsystem::ResolvePlayerStartActor(AController* Controller, FGameplayTag SpawnPointId) const
{
	APRSpawnPoint* SpawnPoint = FindSpawnPointById(SpawnPointId);
	if (!IsValid(SpawnPoint))
	{
		return nullptr;
	}

	const int32 PlayerIndex = ResolvePlayerIndex(Controller);
	if (APlayerStart* LinkedPlayerStart = SpawnPoint->GetLinkedPlayerStart(PlayerIndex))
	{
		return LinkedPlayerStart;
	}

	return SpawnPoint;
}

bool UPRRespawnSubsystem::ResolvePlayerRespawnTransform(AController* Controller, FGameplayTag SpawnPointId, FTransform& OutTransform) const
{
	APRSpawnPoint* SpawnPoint = FindSpawnPointById(SpawnPointId);
	if (!IsValid(SpawnPoint))
	{
		return false;
	}

	const int32 PlayerIndex = ResolvePlayerIndex(Controller);
	OutTransform = SpawnPoint->GetSpawnTransform(PlayerIndex);
	return true;
}

/*~ 내부 헬퍼 ~*/

bool UPRRespawnSubsystem::IsServerWorld() const
{
	const UWorld* World = GetWorld();
	return IsValid(World) && World->GetNetMode() != NM_Client;
}

AController* UPRRespawnSubsystem::ResolveControllerFromPlayerState(APlayerState* PlayerState) const
{
	if (!IsValid(PlayerState))
	{
		return nullptr;
	}

	AController* Controller = Cast<AController>(PlayerState->GetOwner());
	if (!IsValid(Controller) && IsValid(PlayerState->GetPawn()))
	{
		Controller = PlayerState->GetPawn()->GetController();
	}

	return Controller;
}

void UPRRespawnSubsystem::DestroyControllerPawn(AController* Controller) const
{
	if (!IsValid(Controller))
	{
		return;
	}

	APawn* ExistingPawn = Controller->GetPawn();
	if (!IsValid(ExistingPawn))
	{
		return;
	}

	// 컨트롤러 연결 해제
	Controller->UnPossess();

	// 기존 Pawn 제거
	ExistingPawn->Destroy();
}

void UPRRespawnSubsystem::NotifyRespawnComplete(AController* Controller) const
{
	APRPlayerController* PlayerController = Cast<APRPlayerController>(Controller);
	if (!IsValid(PlayerController))
	{
		return;
	}

	// 리스폰 완료 FadeIn
	PlayerController->ClientNotifyMapTransition(0.25f, EPRMapTransitionType::RespawnComplete);
}

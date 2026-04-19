// Copyright ProjectR. All Rights Reserved.

#include "PRSessionSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Internationalization/Regex.h"

// ===== 초기화 ===== 

void UPRSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// 네트워크 실패 콜백 등록. ClientTravel 실패·연결 끊김을 여기서 포착
	if (GEngine)
	{
		GEngine->NetworkFailureEvent.AddUObject(this, &UPRSessionSubsystem::HandleNetworkFailure);
	}
}

void UPRSessionSubsystem::Deinitialize()
{
	if (GEngine)
	{
		GEngine->NetworkFailureEvent.RemoveAll(this);
	}

	Super::Deinitialize();
}

// ===== 상태 전이 ===== 

void UPRSessionSubsystem::SetState(EPRSessionState NewState)
{
	if (CurrentState == NewState)
	{
		return;
	}

	CurrentState = NewState;
	OnSessionStateChanged.Broadcast(CurrentState);
}

// ===== Host 경로 ===== 

void UPRSessionSubsystem::StartHost(const FPRHostSessionParams& Params)
{
	if (Params.MapName.IsNone())
	{
		OnSessionFailed.Broadcast(EPRSessionFailReason::MapLoadFailed, TEXT("MapName is empty"));
		return;
	}

	PendingHostParams = Params;
	SetState(EPRSessionState::Hosting);

	// listen 옵션으로 리슨 서버 개시
	const FString Options = FString::Printf(TEXT("listen?MaxPlayers=%d"), Params.MaxPlayers);
	UGameplayStatics::OpenLevel(this, Params.MapName, true, Options);

	// OpenLevel은 비동기이므로 Hosted 전이는 맵 로드 완료 시 GameMode에서 확정한다
	SetState(EPRSessionState::Hosted);
}

// ===== Join 경로 ===== 

bool UPRSessionSubsystem::ValidateAddress(const FString& Address) const
{
	// IPv4(:Port)? 포맷만 허용. 호스트네임은 현재 단계에서 미지원
	const FRegexPattern Pattern(TEXT("^(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})(:\\d{1,5})?$"));
	FRegexMatcher Matcher(Pattern, Address);
	return Matcher.FindNext();
}

void UPRSessionSubsystem::StartJoin(const FPRJoinSessionParams& Params)
{
	if (!ValidateAddress(Params.Address))
	{
		OnSessionFailed.Broadcast(EPRSessionFailReason::InvalidAddress, Params.Address);
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	if (!IsValid(GameInstance))
	{
		OnSessionFailed.Broadcast(EPRSessionFailReason::Unknown, TEXT("GameInstance invalid"));
		return;
	}

	APlayerController* PC = GameInstance->GetFirstLocalPlayerController();
	if (!IsValid(PC))
	{
		OnSessionFailed.Broadcast(EPRSessionFailReason::Unknown, TEXT("PlayerController invalid"));
		return;
	}

	SetState(EPRSessionState::Joining);

	// 접속 시도. 실패는 HandleNetworkFailure로 통지됨
	PC->ClientTravel(Params.Address, TRAVEL_Absolute);

	// Joined 전이는 접속 완료 후 PlayerController가 PostLogin 처리할 때 확정한다
	SetState(EPRSessionState::Joined);
}

// ===== 종료 ===== 

void UPRSessionSubsystem::EndSession()
{
	SetState(EPRSessionState::Leaving);

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		SetState(EPRSessionState::None);
		return;
	}

	// 호스트는 ServerTravel(모든 클라 동반 이동), 게스트는 ClientTravel
	if (World->GetNetMode() == NM_ListenServer || World->GetNetMode() == NM_DedicatedServer)
	{
		World->ServerTravel(MenuMapName.ToString(), true);
	}
	else
	{
		if (APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController())
		{
			PC->ClientTravel(MenuMapName.ToString(), TRAVEL_Absolute);
		}
	}

	SetState(EPRSessionState::None);
}

// ===== 네트워크 실패 ===== 

void UPRSessionSubsystem::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	OnSessionFailed.Broadcast(EPRSessionFailReason::NetworkFailure, ErrorString);
	SetState(EPRSessionState::None);
}

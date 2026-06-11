// Copyright ProjectR. All Rights Reserved.

#include "PRSessionSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/NetDriver.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Internationalization/Regex.h"
#include "Misc/PackageName.h"
#include "ProjectR/System/PRLoadingScreenSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogPRSession, Log, All);

namespace
{
	FString NormalizeMapPackageName(const FString& MapPackageName)
	{
		// PIE 접두사와 오브젝트 이름 접미사를 제거한 비교용 패키지 이름
		FString NormalizedName = UWorld::RemovePIEPrefix(MapPackageName);
		int32 ObjectNameSeparatorIndex = INDEX_NONE;
		if (NormalizedName.FindChar(TEXT('.'), ObjectNameSeparatorIndex))
		{
			NormalizedName.LeftInline(ObjectNameSeparatorIndex);
		}

		return NormalizedName;
	}

	bool IsSameMapForRestart(const UWorld* World, const FString& TargetPackageName)
	{
		if (!IsValid(World) || TargetPackageName.IsEmpty())
		{
			return false;
		}

		const FString NormalizedTargetPackageName = NormalizeMapPackageName(TargetPackageName);
		const FString NormalizedCurrentPackageName = NormalizeMapPackageName(World->GetOutermost()->GetName());
		if (NormalizedCurrentPackageName == NormalizedTargetPackageName)
		{
			return true;
		}

		const FString NormalizedCurrentMapName = NormalizeMapPackageName(World->GetMapName());
		if (NormalizedCurrentMapName == NormalizedTargetPackageName)
		{
			return true;
		}

		// PIE와 Restart URL 경로에서 짧은 맵 이름만 남는 경우를 위한 보조 판정
		const FString CurrentShortName = FPackageName::GetShortName(NormalizedCurrentPackageName);
		const FString CurrentMapShortName = FPackageName::GetShortName(NormalizedCurrentMapName);
		const FString TargetShortName = FPackageName::GetShortName(NormalizedTargetPackageName);
		return CurrentShortName == TargetShortName || CurrentMapShortName == TargetShortName;
	}
}

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

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UPRLoadingScreenSubsystem* LoadingScreen = GameInstance->GetSubsystem<UPRLoadingScreenSubsystem>())
		{
			LoadingScreen->BeginTravelToMap(TEXT("StartHost"), Params.MapName.ToString());
		}
	}

	// listen 옵션으로 리슨 서버 개시. SessionPort 명시로 PIE/Standalone 포트 통일
	const FString Options = FString::Printf(TEXT("listen?MaxPlayers=%d?Port=%d"), Params.MaxPlayers, SessionPort);
	UE_LOG(LogPRSession, Log, TEXT("[StartHost] OpenLevel Map=%s, Options=%s"), *Params.MapName.ToString(), *Options);
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
		UE_LOG(LogPRSession, Warning, TEXT("[StartJoin] InvalidAddress=%s"), *Params.Address);
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

	// 같은 프로세스가 이미 리슨 서버이거나 호스트 상태이면 자기 자신으로 ClientTravel 불가
	UWorld* World = GetWorld();
	const ENetMode NetMode = IsValid(World) ? World->GetNetMode() : NM_Standalone;
	if (NetMode == NM_ListenServer || NetMode == NM_DedicatedServer)
	{
		UE_LOG(LogPRSession, Warning, TEXT("[StartJoin] 호스트 프로세스에서 Join 시도 차단 NetMode=%d"), static_cast<int32>(NetMode));
		OnSessionFailed.Broadcast(EPRSessionFailReason::Unknown, TEXT("Cannot join from a hosting instance"));
		return;
	}

	// 메뉴에서 포트 미입력 시 SessionPort로 보정. 호스트와 동일 포트 강제
	FString TravelAddress = Params.Address;
	if (!TravelAddress.Contains(TEXT(":")))
	{
		TravelAddress += FString::Printf(TEXT(":%d"), SessionPort);
	}

	UE_LOG(LogPRSession, Log, TEXT("[StartJoin] ClientTravel Address=%s NetMode=%d PC=%s"),
		*TravelAddress, static_cast<int32>(NetMode), *PC->GetName());

	SetState(EPRSessionState::Joining);

	if (UPRLoadingScreenSubsystem* LoadingScreen = GameInstance->GetSubsystem<UPRLoadingScreenSubsystem>())
	{
		LoadingScreen->BeginTravel(TEXT("StartJoin"));
	}

	// 접속 시도. 실패는 HandleNetworkFailure로 통지됨
	// ClientTravel은 비동기. 실제 Joined 전이는 새 맵에서 PostLogin/WelcomeReceived 시점에 확정해야 함
	PC->ClientTravel(TravelAddress, TRAVEL_Absolute);
}

// ===== ServerTravel =====

bool UPRSessionSubsystem::ServerTravelToMap(TSoftObjectPtr<UWorld> MapAsset, bool bAbsolute)
{
	if (MapAsset.IsNull())
	{
		OnSessionFailed.Broadcast(EPRSessionFailReason::MapLoadFailed, TEXT("MapAsset is null"));
		return false;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		OnSessionFailed.Broadcast(EPRSessionFailReason::Unknown, TEXT("World invalid"));
		return false;
	}

	const ENetMode NetMode = World->GetNetMode();
	if (NetMode != NM_ListenServer && NetMode != NM_DedicatedServer && NetMode != NM_Standalone)
	{
		OnSessionFailed.Broadcast(EPRSessionFailReason::Unknown, TEXT("ServerTravel requires server authority"));
		return false;
	}

	if (World->IsInSeamlessTravel())
	{
		// 이미 진행 중인 SeamlessTravel에 대한 중복 요청 차단
		OnSessionFailed.Broadcast(EPRSessionFailReason::Unknown, TEXT("ServerTravel already in progress"));
		return false;
	}

	// 소프트 참조의 LongPackageName을 ServerTravel URL로 사용 (예: /Game/.../L_MyMap)
	const FString PackageName = MapAsset.GetLongPackageName();
	if (PackageName.IsEmpty())
	{
		OnSessionFailed.Broadcast(EPRSessionFailReason::MapLoadFailed, TEXT("MapAsset package name empty"));
		return false;
	}

	const FString CurrentPackageName = NormalizeMapPackageName(World->GetOutermost()->GetName());
	if (IsSameMapForRestart(World, PackageName))
	{
		// 현재 맵 재진입은 플레이어 리스폰과 동일한 Restart URL 사용
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (UPRLoadingScreenSubsystem* LoadingScreen = GameInstance->GetSubsystem<UPRLoadingScreenSubsystem>())
			{
				LoadingScreen->BeginTravelToMap(TEXT("ServerTravelRestart"), CurrentPackageName);
			}
		}

		const bool bRestartStarted = World->ServerTravel(TEXT("?Restart"), false);
		if (!bRestartStarted)
		{
			UE_LOG(LogPRSession, Warning, TEXT("[ServerTravelToMap] Restart failed. Current=%s, Target=%s"),
				*CurrentPackageName,
				*PackageName);
			OnSessionFailed.Broadcast(EPRSessionFailReason::MapLoadFailed, TEXT("?Restart"));
		}

		return bRestartStarted;
	}

	const FString TravelURL = PackageName;
	const bool bResolvedAbsolute = bAbsolute;

	// 다른 맵 이동은 소프트 참조 패키지 이름으로 ServerTravel 실행
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UPRLoadingScreenSubsystem* LoadingScreen = GameInstance->GetSubsystem<UPRLoadingScreenSubsystem>())
		{
			LoadingScreen->BeginTravelToMap(TEXT("ServerTravelToMap"), PackageName);
		}
	}

	const bool bTravelStarted = World->ServerTravel(TravelURL, bResolvedAbsolute);
	if (!bTravelStarted)
	{
		UE_LOG(LogPRSession, Warning, TEXT("[ServerTravelToMap] ServerTravel failed. URL=%s, Current=%s"),
			*TravelURL,
			*CurrentPackageName);
		OnSessionFailed.Broadcast(EPRSessionFailReason::MapLoadFailed, TravelURL);
	}

	return bTravelStarted;
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
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (UPRLoadingScreenSubsystem* LoadingScreen = GameInstance->GetSubsystem<UPRLoadingScreenSubsystem>())
			{
				LoadingScreen->BeginTravelToMap(TEXT("EndSessionHost"), MenuMapName.ToString());
			}
		}

		World->ServerTravel(MenuMapName.ToString(), true);
	}
	else
	{
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (UPRLoadingScreenSubsystem* LoadingScreen = GameInstance->GetSubsystem<UPRLoadingScreenSubsystem>())
			{
				LoadingScreen->BeginTravelToMap(TEXT("EndSessionClient"), MenuMapName.ToString());
			}
		}

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
	UE_LOG(LogPRSession, Warning, TEXT("[HandleNetworkFailure] FailureType=%d, Error=%s"),
		static_cast<int32>(FailureType), *ErrorString);

	OnSessionFailed.Broadcast(EPRSessionFailReason::NetworkFailure, ErrorString);
	SetState(EPRSessionState::None);
}

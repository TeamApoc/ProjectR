// Copyright ProjectR. All Rights Reserved.

#include "PRGameInstance.h"
#include "PRSessionSubsystem.h"
#include "PRSaveGame.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	constexpr int32 LocalCharacterSaveUserIndex = 0;
	constexpr int32 MinLocalCharacterSlotIndex = 1;
	constexpr int32 MaxLocalCharacterSlotIndex = 4;
}

// ===== 초기화 =====

void UPRGameInstance::Init()
{
	Super::Init();

	// 저장 파일 부재 시 기본 1번 슬롯 생성
	EnsureInitialLocalCharacterSave();
}

void UPRGameInstance::Shutdown()
{
	Super::Shutdown();
}

// ===== 세션 진입점 =====

void UPRGameInstance::HostSession(const FPRHostSessionParams& Params)
{
	UPRSessionSubsystem* Session = GetSubsystem<UPRSessionSubsystem>();
	if (!IsValid(Session))
	{
		return;
	}

	Session->StartHost(Params);
}

void UPRGameInstance::JoinSession(const FPRJoinSessionParams& Params)
{
	UPRSessionSubsystem* Session = GetSubsystem<UPRSessionSubsystem>();
	if (!IsValid(Session))
	{
		return;
	}

	Session->StartJoin(Params);
}

void UPRGameInstance::LeaveSession()
{
	UPRSessionSubsystem* Session = GetSubsystem<UPRSessionSubsystem>();
	if (!IsValid(Session))
	{
		return;
	}

	Session->EndSession();
}

void UPRGameInstance::ServerTravelToMap(TSoftObjectPtr<UWorld> MapAsset, bool bAbsolute)
{
	UPRSessionSubsystem* Session = GetSubsystem<UPRSessionSubsystem>();
	if (!IsValid(Session))
	{
		return;
	}

	Session->ServerTravelToMap(MapAsset, bAbsolute);
}

// ===== 맵 이동 Waypoint 컨텍스트 =====

void UPRGameInstance::SetPendingTravelWaypointId(FGameplayTag WaypointId)
{
	PendingTravelWaypointId = WaypointId;
}

FGameplayTag UPRGameInstance::ConsumePendingTravelWaypointId()
{
	// 일회성 컨텍스트 소비
	const FGameplayTag ConsumedWaypointId = PendingTravelWaypointId;
	PendingTravelWaypointId = FGameplayTag();
	return ConsumedWaypointId;
}

void UPRGameInstance::SetPendingWorldSaveData(const FPRWorldSaveData& WorldSaveData)
{
	PendingWorldSaveData = WorldSaveData;
	bHasPendingWorldSaveData = true;
}

bool UPRGameInstance::ConsumePendingWorldSaveData(FPRWorldSaveData& OutWorldSaveData)
{
	if (!bHasPendingWorldSaveData)
	{
		return false;
	}

	OutWorldSaveData = PendingWorldSaveData;
	PendingWorldSaveData = FPRWorldSaveData();
	bHasPendingWorldSaveData = false;
	return true;
}

// ===== 세이브 연동 (스켈레톤) =====

bool UPRGameInstance::LoadLocalCharacter(FName SlotName)
{
	if (SlotName.IsNone())
	{
		return false;
	}

	// 지정 슬롯 이름의 SaveGame 파일 로드
	USaveGame* LoadedSaveGame = UGameplayStatics::LoadGameFromSlot(SlotName.ToString(), LocalCharacterSaveUserIndex);
	UPRSaveGame* LoadedPRSaveGame = Cast<UPRSaveGame>(LoadedSaveGame);
	if (!IsValid(LoadedPRSaveGame))
	{
		return false;
	}

	LocalCharacter = LoadedPRSaveGame->CharacterSaveData;
	return true;
}

bool UPRGameInstance::SaveLocalCharacter(FName SlotName)
{
	if (SlotName.IsNone())
	{
		return false;
	}

	// 현재 로컬 캐릭터 상태를 SaveGame 객체에 기록
	UPRSaveGame* NewSaveGame = Cast<UPRSaveGame>(UGameplayStatics::CreateSaveGameObject(UPRSaveGame::StaticClass()));
	if (!IsValid(NewSaveGame))
	{
		return false;
	}

	NewSaveGame->CharacterSaveData = LocalCharacter;
	return UGameplayStatics::SaveGameToSlot(NewSaveGame, SlotName.ToString(), LocalCharacterSaveUserIndex);
}

bool UPRGameInstance::DoesLocalCharacterSaveExist(int32 SlotIndex) const
{
	if (!IsValidLocalCharacterSlotIndex(SlotIndex))
	{
		return false;
	}

	return UGameplayStatics::DoesSaveGameExist(BuildLocalCharacterSlotName(SlotIndex), LocalCharacterSaveUserIndex);
}

bool UPRGameInstance::LoadLocalCharacterSlot(int32 SlotIndex)
{
	if (!IsValidLocalCharacterSlotIndex(SlotIndex))
	{
		return false;
	}

	const FString SlotName = BuildLocalCharacterSlotName(SlotIndex);
	return LoadLocalCharacter(FName(*SlotName));
}

bool UPRGameInstance::TryGetLocalCharacterSaveSlotData(int32 SlotIndex, FPRCharacterSaveData& OutSaveData) const
{
	if (!IsValidLocalCharacterSlotIndex(SlotIndex))
	{
		return false;
	}

	const FString SlotName = BuildLocalCharacterSlotName(SlotIndex);
	if (!UGameplayStatics::DoesSaveGameExist(SlotName, LocalCharacterSaveUserIndex))
	{
		return false;
	}

	// 메뉴 표시 전용 저장 데이터 조회
	USaveGame* LoadedSaveGame = UGameplayStatics::LoadGameFromSlot(SlotName, LocalCharacterSaveUserIndex);
	const UPRSaveGame* LoadedPRSaveGame = Cast<UPRSaveGame>(LoadedSaveGame);
	if (!IsValid(LoadedPRSaveGame))
	{
		return false;
	}

	OutSaveData = LoadedPRSaveGame->CharacterSaveData;
	return true;
}

bool UPRGameInstance::SaveLocalCharacterSlot(int32 SlotIndex)
{
	if (!IsValidLocalCharacterSlotIndex(SlotIndex))
	{
		return false;
	}

	const FString SlotName = BuildLocalCharacterSlotName(SlotIndex);
	return SaveLocalCharacter(FName(*SlotName));
}

bool UPRGameInstance::EnsureInitialLocalCharacterSave()
{
	for (int32 SlotIndex = MinLocalCharacterSlotIndex; SlotIndex <= MaxLocalCharacterSlotIndex; ++SlotIndex)
	{
		if (DoesLocalCharacterSaveExist(SlotIndex))
		{
			return true;
		}
	}

	// 모든 슬롯이 비어 있는 최초 실행 상태
	LocalCharacter = FPRCharacterSaveData();
	return SaveLocalCharacterSlot(MinLocalCharacterSlotIndex);
}

void UPRGameInstance::ApplyRewardGrant(const FPRRewardGrant& Grant)
{
	// 즉시 지급. 경험치는 로컬 캐릭터에 바로 반영
	LocalCharacter.Experience += Grant.Experience;
	// 재화는 인벤토리 시스템 구현 후 반영 (현재는 스텁)
}

FString UPRGameInstance::BuildLocalCharacterSlotName(int32 SlotIndex) const
{
	return FString::Printf(TEXT("CharacterSlot_%d"), SlotIndex);
}

bool UPRGameInstance::IsValidLocalCharacterSlotIndex(int32 SlotIndex) const
{
	return SlotIndex >= MinLocalCharacterSlotIndex && SlotIndex <= MaxLocalCharacterSlotIndex;
}

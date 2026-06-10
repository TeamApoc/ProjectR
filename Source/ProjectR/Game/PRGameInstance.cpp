// Copyright ProjectR. All Rights Reserved.

#include "PRGameInstance.h"
#include "PRGameStateBase.h"
#include "PRSessionSubsystem.h"
#include "PRSaveGame.h"
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
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

	// 프로젝트 기본 그래픽 품질 프로필 적용
	ApplyGraphicsQualityProfile(DefaultGraphicsQualityProfile, false);

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

bool UPRGameInstance::ApplyGraphicsQualityProfile(EPRGraphicsQualityProfile QualityProfile, bool bSaveSettings)
{
	if (!GEngine)
	{
		return false;
	}

	UGameUserSettings* GameUserSettings = GEngine->GetGameUserSettings();
	if (!IsValid(GameUserSettings))
	{
		return false;
	}

	// EPRGraphicsQualityProfile 값은 UE Scalability 전체 품질 인덱스와 동일한 순서
	const int32 ScalabilityLevel = static_cast<int32>(QualityProfile);
	GameUserSettings->SetOverallScalabilityLevel(ScalabilityLevel);
	GameUserSettings->ApplySettings(false);

	if (bSaveSettings)
	{
		// 사용자가 명시적으로 선택한 품질 프로필의 영구 저장
		GameUserSettings->SaveSettings();
	}

	return true;
}

bool UPRGameInstance::ServerTravelToMap(TSoftObjectPtr<UWorld> MapAsset, bool bAbsolute)
{
	UPRSessionSubsystem* Session = GetSubsystem<UPRSessionSubsystem>();
	if (!IsValid(Session))
	{
		return false;
	}

	return Session->ServerTravelToMap(MapAsset, bAbsolute);
}

// ===== 맵 이동 SpawnPoint 컨텍스트 =====

void UPRGameInstance::SetPendingTravelSpawnPointId(FGameplayTag SpawnPointId)
{
	PendingTravelSpawnPointId = SpawnPointId;
}

FGameplayTag UPRGameInstance::ConsumePendingTravelSpawnPointId()
{
	// 일회성 컨텍스트 소비
	const FGameplayTag ConsumedSpawnPointId = PendingTravelSpawnPointId;
	PendingTravelSpawnPointId = FGameplayTag();
	return ConsumedSpawnPointId;
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

	LocalCharacterSave = LoadedPRSaveGame->CharacterSaveData;
	LocalWorldSave = LoadedPRSaveGame->WorldSaveData;
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

	NewSaveGame->CharacterSaveData = LocalCharacterSave;
	NewSaveGame->WorldSaveData = LocalWorldSave;
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
	const bool bLoaded = LoadLocalCharacter(FName(*SlotName));
	if (bLoaded)
	{
		// 플레이 중 저장 대상 슬롯 기록
		ActiveLocalCharacterSlotIndex = SlotIndex;
	}

	return bLoaded;
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

	RefreshLocalWorldSaveFromGameState();

	const FString SlotName = BuildLocalCharacterSlotName(SlotIndex);
	const bool bSaved = SaveLocalCharacter(FName(*SlotName));
	if (bSaved)
	{
		// 플레이 중 저장 대상 슬롯 기록
		ActiveLocalCharacterSlotIndex = SlotIndex;
	}

	return bSaved;
}

bool UPRGameInstance::DeleteLocalCharacterSaveSlot(int32 SlotIndex)
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

	const bool bDeleted = UGameplayStatics::DeleteGameInSlot(SlotName, LocalCharacterSaveUserIndex);
	if (bDeleted && ActiveLocalCharacterSlotIndex == SlotIndex)
	{
		// 삭제된 슬롯을 세션 진입용 캐릭터로 계속 사용하지 않도록 활성 슬롯 초기화
		ActiveLocalCharacterSlotIndex = INDEX_NONE;
		LocalCharacterSave = FPRCharacterSaveData();
		LocalWorldSave = FPRWorldSaveData();
	}

	return bDeleted;
}

bool UPRGameInstance::SaveActiveLocalCharacterSlot()
{
	if (!HasActiveLocalCharacterSlot())
	{
		return false;
	}

	return SaveLocalCharacterSlot(ActiveLocalCharacterSlotIndex);
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
	LocalCharacterSave = FPRCharacterSaveData();
	const bool bSaved = SaveLocalCharacterSlot(MinLocalCharacterSlotIndex);
	if (!bSaved)
	{
		ActiveLocalCharacterSlotIndex = INDEX_NONE;
	}

	return bSaved;
}

bool UPRGameInstance::EnsureEmptyLocalCharacterSaveSlot()
{
	int32 FirstEmptySlotIndex = INDEX_NONE;
	for (int32 SlotIndex = MinLocalCharacterSlotIndex; SlotIndex <= MaxLocalCharacterSlotIndex; ++SlotIndex)
	{
		if (!DoesLocalCharacterSaveExist(SlotIndex))
		{
			if (FirstEmptySlotIndex == INDEX_NONE)
			{
				// 가장 낮은 빈 슬롯 후보 보관
				FirstEmptySlotIndex = SlotIndex;
			}
			continue;
		}

		FPRCharacterSaveData SaveData;
		if (TryGetLocalCharacterSaveSlotData(SlotIndex, SaveData) && IsDefaultLocalCharacterSaveData(SaveData))
		{
			// 이미 신규 게임용 기본 슬롯 존재
			return true;
		}
	}

	if (FirstEmptySlotIndex == INDEX_NONE)
	{
		// 모든 슬롯 사용 중
		return true;
	}

	const FPRCharacterSaveData PreviousLocalCharacter = LocalCharacterSave;
	const FPRWorldSaveData PreviousLocalWorldSave = LocalWorldSave;
	const int32 PreviousActiveLocalCharacterSlotIndex = ActiveLocalCharacterSlotIndex;
	LocalCharacterSave = FPRCharacterSaveData();
	LocalWorldSave = FPRWorldSaveData();

	// 기존 진행 슬롯을 덮어쓰지 않는 첫 빈 슬롯 저장
	const bool bSaved = SaveLocalCharacterSlot(FirstEmptySlotIndex);
	LocalCharacterSave = PreviousLocalCharacter;
	LocalWorldSave = PreviousLocalWorldSave;
	ActiveLocalCharacterSlotIndex = PreviousActiveLocalCharacterSlotIndex;
	return bSaved;
}

bool UPRGameInstance::EnsureLocalCharacterReadyForSession()
{
	// 메뉴 미경유 PIE 대비 최소 저장 파일 보장
	EnsureInitialLocalCharacterSave();

	if (HasActiveLocalCharacterSlot())
	{
		// 메뉴에서 선택한 슬롯 우선 재로드
		if (LoadLocalCharacterSlot(ActiveLocalCharacterSlotIndex))
		{
			return true;
		}

		// 삭제 또는 손상 슬롯 대비
		ActiveLocalCharacterSlotIndex = INDEX_NONE;
	}

	for (int32 SlotIndex = MinLocalCharacterSlotIndex; SlotIndex <= MaxLocalCharacterSlotIndex; ++SlotIndex)
	{
		if (!DoesLocalCharacterSaveExist(SlotIndex))
		{
			continue;
		}

		// 직접 플레이 시작 시 첫 로드 가능 슬롯 사용
		if (LoadLocalCharacterSlot(SlotIndex))
		{
			return true;
		}
	}

	// 모든 저장 슬롯 로드 실패 시 런타임 기본 페이로드 사용
	LocalCharacterSave = FPRCharacterSaveData();
	LocalWorldSave = FPRWorldSaveData();
	ActiveLocalCharacterSlotIndex = INDEX_NONE;
	return true;
}

void UPRGameInstance::ApplyRewardGrant(const FPRRewardGrant& Grant)
{
	// 즉시 지급. 경험치는 로컬 캐릭터에 바로 반영
	LocalCharacterSave.Experience += Grant.Experience;
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

bool UPRGameInstance::IsDefaultLocalCharacterSaveData(const FPRCharacterSaveData& SaveData) const
{
	const FPRCharacterSaveData DefaultSaveData;
	// 구조체 전체 기본값 비교로 신규 게임 슬롯 판별
	return FPRCharacterSaveData::StaticStruct()->CompareScriptStruct(&SaveData, &DefaultSaveData, PPF_None);
}

void UPRGameInstance::RefreshLocalWorldSaveFromGameState()
{
	UWorld* World = GetWorld();
	const APRGameStateBase* GameState = IsValid(World) ? World->GetGameState<APRGameStateBase>() : nullptr;
	if (!IsValid(GameState))
	{
		return;
	}

	// 저장 직전 서버 GameState의 월드 진행 상태 스냅샷 반영
	LocalWorldSave = GameState->MakeWorldSaveData();
}

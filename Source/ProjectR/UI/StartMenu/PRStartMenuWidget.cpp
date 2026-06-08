// Copyright ProjectR. All Rights Reserved.

#include "PRStartMenuWidget.h"

#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "ProjectR/Game/PRGameInstance.h"
#include "ProjectR/Game/PRMenuGameMode.h"
#include "ProjectR/Game/PRSessionSubsystem.h"
#include "ProjectR/ItemSystem/Data/PREquipmentDataAsset.h"
#include "ProjectR/ItemSystem/Data/PRWeaponDataAsset.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "ProjectR/UI/Growth/PRPlayerStatsPanelWidget.h"
#include "ProjectR/UI/Inventory/PRInventoryItemSlotViewDataBuilder.h"
#include "ProjectR/UI/Inventory/PRItemSlotWidget.h"
#include "ProjectR/UI/Preview/PRCharacterPreviewWidget.h"

/*~ UUserWidget Interface ~*/

void UPRStartMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 메뉴 세이브 목록과 세션 진입 캐릭터 페이로드를 위한 최소 슬롯 보장
	if (UPRGameInstance* GameInstance = GetProjectRGameInstance())
	{
		GameInstance->EnsureInitialLocalCharacterSave();
		// 기존 진행 세이브와 별도로 유지되는 신규 게임 후보 슬롯
		GameInstance->EnsureEmptyLocalCharacterSaveSlot();
	}

	CacheWidgetLists();
	BindSaveSlotButtonEvents();
	BindStartButtonEvents();
	BindDeleteSaveSlotButtonEvents();
	// 세션 델리게이트 초기 상태 반영 전 입력 기본값 선적용
	ConfigureSessionInputDefaults();
	BindSessionEvents();
	RefreshSaveSlotButtons();
	SelectFirstAvailableSaveSlot();
}

void UPRStartMenuWidget::NativeDestruct()
{
	UnbindSessionEvents();
	UnbindDeleteSaveSlotButtonEvents();
	UnbindStartButtonEvents();
	UnbindSaveSlotButtonEvents();

	Super::NativeDestruct();
}

void UPRStartMenuWidget::CacheWidgetLists()
{
	// UMG 고정 슬롯과 1기반 세이브 슬롯 번호의 순서 계약
	SaveSlotButtons.Reset();
	SaveSlotButtons.Add(SaveSlotButton1);
	SaveSlotButtons.Add(SaveSlotButton2);
	SaveSlotButtons.Add(SaveSlotButton3);
	SaveSlotButtons.Add(SaveSlotButton4);

	// 버튼 배열과 동일 인덱스로 접근하는 라벨 캐시
	SaveSlotTexts.Reset();
	SaveSlotTexts.Add(SaveSlotText1);
	SaveSlotTexts.Add(SaveSlotText2);
	SaveSlotTexts.Add(SaveSlotText3);
	SaveSlotTexts.Add(SaveSlotText4);

	// 장비 프리뷰 위젯과 저장 데이터 슬롯 타입의 동일 인덱스 계약
	EquipmentSlotWidgets.Reset();
	EquipmentSlotTypes.Reset();

	EquipmentSlotWidgets.Add(HeadEquipmentSlotWidget);
	EquipmentSlotTypes.Add(EPREquipmentSlotType::Head);

	EquipmentSlotWidgets.Add(BodyEquipmentSlotWidget);
	EquipmentSlotTypes.Add(EPREquipmentSlotType::Body);

	EquipmentSlotWidgets.Add(HandsEquipmentSlotWidget);
	EquipmentSlotTypes.Add(EPREquipmentSlotType::Hands);

	EquipmentSlotWidgets.Add(LegsEquipmentSlotWidget);
	EquipmentSlotTypes.Add(EPREquipmentSlotType::Legs);

	EquipmentSlotWidgets.Add(AmuletEquipmentSlotWidget);
	EquipmentSlotTypes.Add(EPREquipmentSlotType::Amulet);

	EquipmentSlotWidgets.Add(Ring1EquipmentSlotWidget);
	EquipmentSlotTypes.Add(EPREquipmentSlotType::Ring1);

	EquipmentSlotWidgets.Add(Ring2EquipmentSlotWidget);
	EquipmentSlotTypes.Add(EPREquipmentSlotType::Ring2);
}

void UPRStartMenuWidget::BindSaveSlotButtonEvents()
{
	UnbindSaveSlotButtonEvents();

	// BindWidget 이름이 고정된 4개 슬롯 버튼 개별 바인딩
	if (IsValid(SaveSlotButton1))
	{
		SaveSlotButton1->OnClicked.AddDynamic(this, &ThisClass::HandleSaveSlotButton1Clicked);
	}

	if (IsValid(SaveSlotButton2))
	{
		SaveSlotButton2->OnClicked.AddDynamic(this, &ThisClass::HandleSaveSlotButton2Clicked);
	}

	if (IsValid(SaveSlotButton3))
	{
		SaveSlotButton3->OnClicked.AddDynamic(this, &ThisClass::HandleSaveSlotButton3Clicked);
	}

	if (IsValid(SaveSlotButton4))
	{
		SaveSlotButton4->OnClicked.AddDynamic(this, &ThisClass::HandleSaveSlotButton4Clicked);
	}
}

void UPRStartMenuWidget::UnbindSaveSlotButtonEvents()
{
	// NativeConstruct 반복 호출과 위젯 제거 시 중복 델리게이트 방지
	if (IsValid(SaveSlotButton1))
	{
		SaveSlotButton1->OnClicked.RemoveDynamic(this, &ThisClass::HandleSaveSlotButton1Clicked);
	}

	if (IsValid(SaveSlotButton2))
	{
		SaveSlotButton2->OnClicked.RemoveDynamic(this, &ThisClass::HandleSaveSlotButton2Clicked);
	}

	if (IsValid(SaveSlotButton3))
	{
		SaveSlotButton3->OnClicked.RemoveDynamic(this, &ThisClass::HandleSaveSlotButton3Clicked);
	}

	if (IsValid(SaveSlotButton4))
	{
		SaveSlotButton4->OnClicked.RemoveDynamic(this, &ThisClass::HandleSaveSlotButton4Clicked);
	}
}

void UPRStartMenuWidget::BindStartButtonEvents()
{
	UnbindStartButtonEvents();

	if (IsValid(StartButton))
	{
		StartButton->OnClicked.AddDynamic(this, &ThisClass::HandleStartButtonClicked);
	}
}

void UPRStartMenuWidget::UnbindStartButtonEvents()
{
	if (IsValid(StartButton))
	{
		StartButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleStartButtonClicked);
	}
}

void UPRStartMenuWidget::BindDeleteSaveSlotButtonEvents()
{
	UnbindDeleteSaveSlotButtonEvents();

	if (IsValid(DeleteSaveSlotButton))
	{
		DeleteSaveSlotButton->OnClicked.AddDynamic(this, &ThisClass::HandleDeleteSaveSlotButtonClicked);
	}
}

void UPRStartMenuWidget::UnbindDeleteSaveSlotButtonEvents()
{
	if (IsValid(DeleteSaveSlotButton))
	{
		DeleteSaveSlotButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleDeleteSaveSlotButtonClicked);
	}
}

void UPRStartMenuWidget::BindSessionEvents()
{
	UnbindSessionEvents();

	UPRGameInstance* GameInstance = GetProjectRGameInstance();
	if (!IsValid(GameInstance))
	{
		return;
	}

	UPRSessionSubsystem* SessionSubsystem = GameInstance->GetSubsystem<UPRSessionSubsystem>();
	if (!IsValid(SessionSubsystem))
	{
		return;
	}

	// GameInstanceSubsystem 기반 세션 이벤트 수신
	SessionSubsystem->OnSessionStateChanged.AddDynamic(this, &ThisClass::HandleSessionStateChanged);
	SessionSubsystem->OnSessionFailed.AddDynamic(this, &ThisClass::HandleSessionFailed);

	// 이미 진행 중인 Host/Join 상태와 메뉴 재생성 상태 동기화
	HandleSessionStateChanged(SessionSubsystem->GetState());
}

void UPRStartMenuWidget::UnbindSessionEvents()
{
	UPRGameInstance* GameInstance = GetProjectRGameInstance();
	if (!IsValid(GameInstance))
	{
		return;
	}

	UPRSessionSubsystem* SessionSubsystem = GameInstance->GetSubsystem<UPRSessionSubsystem>();
	if (!IsValid(SessionSubsystem))
	{
		return;
	}

	SessionSubsystem->OnSessionStateChanged.RemoveDynamic(this, &ThisClass::HandleSessionStateChanged);
	SessionSubsystem->OnSessionFailed.RemoveDynamic(this, &ThisClass::HandleSessionFailed);
}

void UPRStartMenuWidget::ConfigureSessionInputDefaults()
{
	if (IsValid(HostAddressTextBox))
	{
		// 빈 입력은 Host, 입력값 존재는 Join으로 해석되는 단일 시작 버튼 규칙
		HostAddressTextBox->SetText(FText::GetEmpty());
		HostAddressTextBox->SetHintText(FText::FromString(TEXT("127.0.0.1")));
	}

	RefreshStartButtonEnabled(true);
	RefreshDeleteSaveSlotButtonEnabled();
	RefreshSessionStatusText(FText::GetEmpty());
}

void UPRStartMenuWidget::RefreshSaveSlotButtons()
{
	for (int32 SlotArrayIndex = 0; SlotArrayIndex < SaveSlotButtons.Num(); ++SlotArrayIndex)
	{
		// 배열 인덱스 0과 저장 슬롯 1의 오프셋 변환
		const int32 SlotIndex = SlotArrayIndex + 1;
		const FPRStartMenuSaveSlotViewData ViewData = BuildSaveSlotViewData(SlotIndex);

		if (SaveSlotButtons.IsValidIndex(SlotArrayIndex) && IsValid(SaveSlotButtons[SlotArrayIndex]))
		{
			SaveSlotButtons[SlotArrayIndex]->SetIsEnabled(ViewData.bHasSave);
		}

		if (SaveSlotTexts.IsValidIndex(SlotArrayIndex) && IsValid(SaveSlotTexts[SlotArrayIndex]))
		{
			SaveSlotTexts[SlotArrayIndex]->SetText(BuildSaveSlotDisplayText(ViewData));
		}
	}

	RefreshDeleteSaveSlotButtonEnabled();
}

void UPRStartMenuWidget::RefreshStartButtonEnabled(bool bEnabled)
{
	if (IsValid(StartButton))
	{
		StartButton->SetIsEnabled(bEnabled);
	}
}

void UPRStartMenuWidget::RefreshDeleteSaveSlotButtonEnabled()
{
	if (IsValid(DeleteSaveSlotButton))
	{
		DeleteSaveSlotButton->SetIsEnabled(SelectedSaveSlotIndex != INDEX_NONE);
	}
}

void UPRStartMenuWidget::RefreshSessionStatusText(const FText& StatusText)
{
	if (IsValid(SessionStatusText))
	{
		SessionStatusText->SetText(StatusText);
	}
}

void UPRStartMenuWidget::RefreshSelectedSavePreview(const FPRCharacterSaveData& SaveData)
{
	APRMenuGameMode* MenuGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<APRMenuGameMode>() : nullptr;
	const bool bPreviewRuntimeReady = IsValid(MenuGameMode) && MenuGameMode->ApplyPreviewSaveData(GetOwningPlayer(), SaveData);
	APRPlayerState* PreviewPlayerState = bPreviewRuntimeReady ? MenuGameMode->GetPreviewPlayerState(GetOwningPlayer()) : nullptr;

	if (IsValid(PrimaryWeaponSlotWidget))
	{
		PrimaryWeaponSlotWidget->SetSlotViewData(BuildSavedWeaponSlotViewData(SaveData, EPRWeaponSlotType::Primary));
	}

	if (IsValid(SecondaryWeaponSlotWidget))
	{
		SecondaryWeaponSlotWidget->SetSlotViewData(BuildSavedWeaponSlotViewData(SaveData, EPRWeaponSlotType::Secondary));
	}

	for (int32 SlotIndex = 0; SlotIndex < EquipmentSlotWidgets.Num(); ++SlotIndex)
	{
		if (!EquipmentSlotTypes.IsValidIndex(SlotIndex))
		{
			continue;
		}

		UPRItemSlotWidget* EquipmentSlotWidget = EquipmentSlotWidgets[SlotIndex];
		if (IsValid(EquipmentSlotWidget))
		{
			// 캐시된 슬롯 타입 기준 저장 장비 데이터 매핑
			EquipmentSlotWidget->SetSlotViewData(BuildSavedEquipmentSlotViewData(SaveData, EquipmentSlotTypes[SlotIndex]));
		}
	}

	if (IsValid(CharacterPreviewWidget))
	{
		CharacterPreviewWidget->SetPreviewSources(
			bPreviewRuntimeReady ? MenuGameMode->GetPreviewCharacter(GetOwningPlayer()) : nullptr,
			IsValid(PreviewPlayerState) ? PreviewPlayerState->GetWeaponManagerComponent() : nullptr);
	}

	if (IsValid(PlayerStatsPanelWidget))
	{
		// 실제 프리뷰 ASC 기반 성장 스탯 패널 갱신
		PlayerStatsPanelWidget->SetPlayerStateSource(PreviewPlayerState);
	}
}

void UPRStartMenuWidget::SelectFirstAvailableSaveSlot()
{
	// 자동 선택 우선순위는 낮은 슬롯 번호 기준
	for (int32 SlotIndex = 1; SlotIndex <= 4; ++SlotIndex)
	{
		if (const UPRGameInstance* GameInstance = GetProjectRGameInstance())
		{
			if (GameInstance->DoesLocalCharacterSaveExist(SlotIndex))
			{
				SelectSaveSlot(SlotIndex);
				return;
			}
		}
	}

	SelectedSaveSlotIndex = INDEX_NONE;
	RefreshSaveSlotButtons();
	RefreshSelectedSavePreview(FPRCharacterSaveData());
}

void UPRStartMenuWidget::SelectSaveSlot(int32 SlotIndex)
{
	UPRGameInstance* GameInstance = GetProjectRGameInstance();
	if (!IsValid(GameInstance) || !GameInstance->DoesLocalCharacterSaveExist(SlotIndex))
	{
		return;
	}

	// 세션 참가 시 제출할 GameInstance LocalCharacter 갱신
	if (!GameInstance->LoadLocalCharacterSlot(SlotIndex))
	{
		return;
	}

	FPRCharacterSaveData SaveData;
	// 메뉴 프리뷰 표시 전용 복사본 조회
	if (!GameInstance->TryGetLocalCharacterSaveSlotData(SlotIndex, SaveData))
	{
		return;
	}

	SelectedSaveSlotIndex = SlotIndex;
	RefreshSaveSlotButtons();
	RefreshSelectedSavePreview(SaveData);
}

FPRStartMenuSaveSlotViewData UPRStartMenuWidget::BuildSaveSlotViewData(int32 SlotIndex) const
{
	// 저장 파일이 없거나 로드 실패 시 빈 슬롯 표시 데이터 유지
	FPRStartMenuSaveSlotViewData ViewData;
	ViewData.SlotIndex = SlotIndex;
	ViewData.bSelected = SelectedSaveSlotIndex == SlotIndex;
	ViewData.DisplayName = FText::Format(FText::FromString(TEXT("슬롯 {0}")), FText::AsNumber(SlotIndex));

	FPRCharacterSaveData SaveData;
	const UPRGameInstance* GameInstance = GetProjectRGameInstance();
	if (IsValid(GameInstance) && GameInstance->TryGetLocalCharacterSaveSlotData(SlotIndex, SaveData))
	{
		// SaveGame 조회 결과만 반영하고 선택 캐릭터 상태는 변경하지 않음
		ViewData.bHasSave = true;
		ViewData.CharacterLevel = SaveData.Level;
		ViewData.DisplayName = SaveData.DisplayName.IsEmpty()
			? ViewData.DisplayName
			: FText::FromString(SaveData.DisplayName);
	}

	return ViewData;
}

FText UPRStartMenuWidget::BuildSaveSlotDisplayText(const FPRStartMenuSaveSlotViewData& ViewData) const
{
	if (!ViewData.bHasSave)
	{
		return FText::Format(FText::FromString(TEXT("슬롯 {0} - 비어 있음")), FText::AsNumber(ViewData.SlotIndex));
	}

	const FText SelectionPrefix = ViewData.bSelected
		? FText::FromString(TEXT("[선택] "))
		: FText::GetEmpty();
	return FText::Format(
		FText::FromString(TEXT("{0}슬롯 {1} - {2} Lv.{3}")),
		SelectionPrefix,
		FText::AsNumber(ViewData.SlotIndex),
		ViewData.DisplayName,
		FText::AsNumber(ViewData.CharacterLevel));
}

FPRInventoryItemSlotViewData UPRStartMenuWidget::BuildSavedWeaponSlotViewData(const FPRCharacterSaveData& SaveData, EPRWeaponSlotType SlotType) const
{
	if (UPRWeaponDataAsset* WeaponData = ResolveSavedWeaponData(SaveData, SlotType))
	{
		// 시작 메뉴 프리뷰에서는 장착 완료 상태로 표시
		return UPRInventoryItemSlotViewDataBuilder::BuildWeaponDataViewData(WeaponData, true);
	}

	return UPRInventoryItemSlotViewDataBuilder::BuildEmptyWeaponSlotViewData(SlotType);
}

FPRInventoryItemSlotViewData UPRStartMenuWidget::BuildSavedEquipmentSlotViewData(const FPRCharacterSaveData& SaveData, EPREquipmentSlotType SlotType) const
{
	if (UPREquipmentDataAsset* EquipmentData = ResolveSavedEquipmentData(SaveData, SlotType))
	{
		// 시작 메뉴 프리뷰에서는 장착 완료 상태로 표시
		return UPRInventoryItemSlotViewDataBuilder::BuildEquipmentDataViewData(EquipmentData, SlotType, true);
	}

	return UPRInventoryItemSlotViewDataBuilder::BuildEmptyEquipmentSlotViewData(SlotType);
}

UPREquipmentDataAsset* UPRStartMenuWidget::ResolveSavedEquipmentData(const FPRCharacterSaveData& SaveData, EPREquipmentSlotType SlotType) const
{
	for (const FPREquipmentSlotSaveEntry& EquippedSlot : SaveData.Equipment.EquippedSlots)
	{
		if (EquippedSlot.SlotType != SlotType)
		{
			continue;
		}

		if (!SaveData.Inventory.Equipments.IsValidIndex(EquippedSlot.EquipmentItemIndex))
		{
			return nullptr;
		}

		// 장착 슬롯 엔트리의 인벤토리 저장 배열 인덱스 역참조
		const FPREquipmentItemSaveEntry& EquipmentEntry = SaveData.Inventory.Equipments[EquippedSlot.EquipmentItemIndex];
		return EquipmentEntry.EquipmentData.LoadSynchronous();
	}

	return nullptr;
}

UPRWeaponDataAsset* UPRStartMenuWidget::ResolveSavedWeaponData(const FPRCharacterSaveData& SaveData, EPRWeaponSlotType SlotType) const
{
	int32 WeaponIndex = INDEX_NONE;
	if (SlotType == EPRWeaponSlotType::Primary)
	{
		WeaponIndex = SaveData.WeaponManager.PrimaryWeaponIndex;
	}
	else if (SlotType == EPRWeaponSlotType::Secondary)
	{
		WeaponIndex = SaveData.WeaponManager.SecondaryWeaponIndex;
	}

	if (!SaveData.Inventory.Weapons.IsValidIndex(WeaponIndex))
	{
		return nullptr;
	}

	// 무기 매니저 저장 인덱스의 인벤토리 저장 배열 역참조
	const FPRWeaponItemSaveEntry& WeaponEntry = SaveData.Inventory.Weapons[WeaponIndex];
	return WeaponEntry.WeaponData.LoadSynchronous();
}

UPRGameInstance* UPRStartMenuWidget::GetProjectRGameInstance() const
{
	return Cast<UPRGameInstance>(GetGameInstance());
}

bool UPRStartMenuWidget::HasAnyLocalCharacterSave() const
{
	const UPRGameInstance* GameInstance = GetProjectRGameInstance();
	if (!IsValid(GameInstance))
	{
		return false;
	}

	for (int32 SlotIndex = 1; SlotIndex <= 4; ++SlotIndex)
	{
		if (GameInstance->DoesLocalCharacterSaveExist(SlotIndex))
		{
			return true;
		}
	}

	return false;
}

void UPRStartMenuWidget::HandleStartButtonClicked()
{
	UPRGameInstance* GameInstance = GetProjectRGameInstance();
	if (!IsValid(GameInstance))
	{
		RefreshSessionStatusText(FText::FromString(TEXT("게임 인스턴스 오류")));
		return;
	}

	if (SelectedSaveSlotIndex == INDEX_NONE)
	{
		// 기본 세이브 자동 생성 이후 선택 상태 누락 보정
		SelectFirstAvailableSaveSlot();
	}

	if (SelectedSaveSlotIndex == INDEX_NONE)
	{
		RefreshSessionStatusText(FText::FromString(TEXT("선택 가능한 세이브 없음")));
		return;
	}

	const FString Address = IsValid(HostAddressTextBox)
		? HostAddressTextBox->GetText().ToString().TrimStartAndEnd()
		: FString();

	// HostSession과 JoinSession의 즉시 실패 이벤트 전까지 중복 클릭 차단
	RefreshStartButtonEnabled(false);

	if (Address.IsEmpty())
	{
		FPRHostSessionParams HostParams;
		// 세션 서브시스템 OpenLevel 경로에 전달할 리슨 서버 설정
		HostParams.MapName = HostMapName;
		HostParams.MaxPlayers = HostMaxPlayers;

		RefreshSessionStatusText(FText::FromString(TEXT("세션 개설 중")));
		GameInstance->HostSession(HostParams);
		return;
	}

	FPRJoinSessionParams JoinParams;
	// 세션 서브시스템 내부에서 IPv4 형식 검증과 기본 포트 보정 처리
	JoinParams.Address = Address;

	RefreshSessionStatusText(FText::FromString(TEXT("세션 참가 중")));
	GameInstance->JoinSession(JoinParams);
}

void UPRStartMenuWidget::HandleDeleteSaveSlotButtonClicked()
{
	UPRGameInstance* GameInstance = GetProjectRGameInstance();
	if (!IsValid(GameInstance) || SelectedSaveSlotIndex == INDEX_NONE)
	{
		RefreshSessionStatusText(FText::FromString(TEXT("삭제할 세이브 없음")));
		return;
	}

	const int32 SlotIndexToDelete = SelectedSaveSlotIndex;
	if (!GameInstance->DeleteLocalCharacterSaveSlot(SlotIndexToDelete))
	{
		RefreshSessionStatusText(FText::FromString(TEXT("세이브 삭제 실패")));
		return;
	}

	SelectedSaveSlotIndex = INDEX_NONE;
	if (!HasAnyLocalCharacterSave())
	{
		// 마지막 저장 파일 삭제 이후 메뉴 시작 흐름 유지를 위한 기본 슬롯 보장
		GameInstance->EnsureInitialLocalCharacterSave();
	}

	RefreshSaveSlotButtons();
	SelectFirstAvailableSaveSlot();
	RefreshSessionStatusText(FText::FromString(TEXT("세이브 삭제 완료")));
}

void UPRStartMenuWidget::HandleSessionStateChanged(EPRSessionState NewState)
{
	// 비동기 맵 이동과 ClientTravel 중복 입력 방지 상태
	const bool bSessionPending =
		NewState == EPRSessionState::Hosting
		|| NewState == EPRSessionState::Joining
		|| NewState == EPRSessionState::Leaving;
	RefreshStartButtonEnabled(!bSessionPending);

	if (NewState == EPRSessionState::Hosting)
	{
		RefreshSessionStatusText(FText::FromString(TEXT("세션 개설 중")));
	}
	else if (NewState == EPRSessionState::Hosted)
	{
		RefreshSessionStatusText(FText::FromString(TEXT("세션 개설 완료")));
	}
	else if (NewState == EPRSessionState::Joining)
	{
		RefreshSessionStatusText(FText::FromString(TEXT("세션 참가 중")));
	}
	else if (NewState == EPRSessionState::Joined)
	{
		RefreshSessionStatusText(FText::FromString(TEXT("세션 참가 완료")));
	}
	else if (NewState == EPRSessionState::Leaving)
	{
		RefreshSessionStatusText(FText::FromString(TEXT("세션 종료 중")));
	}
	else
	{
		RefreshSessionStatusText(FText::GetEmpty());
	}
}

void UPRStartMenuWidget::HandleSessionFailed(EPRSessionFailReason Reason, FString Detail)
{
	// 실패 통지는 네트워크 레이어와 주소 검증 경로의 공통 복구 지점
	RefreshStartButtonEnabled(true);
	UE_LOG(LogTemp, Warning, TEXT("[StartMenu] SessionFailed Reason=%d Detail=%s"),
		static_cast<int32>(Reason), *Detail);

	if (Reason == EPRSessionFailReason::InvalidAddress)
	{
		RefreshSessionStatusText(FText::FromString(TEXT("IP 형식 오류")));
	}
	else if (Reason == EPRSessionFailReason::NetworkFailure)
	{
		RefreshSessionStatusText(FText::FromString(TEXT("네트워크 연결 실패")));
	}
	else if (Reason == EPRSessionFailReason::MapLoadFailed)
	{
		RefreshSessionStatusText(FText::FromString(TEXT("맵 로딩 실패")));
	}
	else if (Reason == EPRSessionFailReason::VersionMismatch)
	{
		RefreshSessionStatusText(FText::FromString(TEXT("세이브 버전 불일치")));
	}
	else if (Reason == EPRSessionFailReason::CharacterRejected)
	{
		RefreshSessionStatusText(FText::FromString(TEXT("캐릭터 데이터 거부")));
	}
	else
	{
		RefreshSessionStatusText(FText::FromString(TEXT("세션 처리 실패")));
	}
}

void UPRStartMenuWidget::HandleSaveSlotButton1Clicked()
{
	SelectSaveSlot(1);
}

void UPRStartMenuWidget::HandleSaveSlotButton2Clicked()
{
	SelectSaveSlot(2);
}

void UPRStartMenuWidget::HandleSaveSlotButton3Clicked()
{
	SelectSaveSlot(3);
}

void UPRStartMenuWidget::HandleSaveSlotButton4Clicked()
{
	SelectSaveSlot(4);
}

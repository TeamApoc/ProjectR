// Copyright ProjectR. All Rights Reserved.

#include "PRStartMenuWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "ProjectR/Game/PRGameInstance.h"
#include "ProjectR/ItemSystem/Data/PREquipmentDataAsset.h"
#include "ProjectR/ItemSystem/Data/PRWeaponDataAsset.h"
#include "ProjectR/UI/Inventory/PRInventoryItemSlotViewDataBuilder.h"
#include "ProjectR/UI/Inventory/PRItemSlotWidget.h"
#include "ProjectR/UI/Preview/PRCharacterPreviewWidget.h"

/*~ UUserWidget Interface ~*/

void UPRStartMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 세이브 파일 부재 시 기본 슬롯 보장
	if (UPRGameInstance* GameInstance = GetProjectRGameInstance())
	{
		GameInstance->EnsureInitialLocalCharacterSave();
	}

	CacheWidgetLists();
	BindSaveSlotButtonEvents();
	RefreshSaveSlotButtons();
	SelectFirstAvailableSaveSlot();
}

void UPRStartMenuWidget::NativeDestruct()
{
	UnbindSaveSlotButtonEvents();

	Super::NativeDestruct();
}

void UPRStartMenuWidget::CacheWidgetLists()
{
	// 세이브 슬롯 버튼 순서 캐시
	SaveSlotButtons.Reset();
	SaveSlotButtons.Add(SaveSlotButton1);
	SaveSlotButtons.Add(SaveSlotButton2);
	SaveSlotButtons.Add(SaveSlotButton3);
	SaveSlotButtons.Add(SaveSlotButton4);

	// 세이브 슬롯 텍스트 순서 캐시
	SaveSlotTexts.Reset();
	SaveSlotTexts.Add(SaveSlotText1);
	SaveSlotTexts.Add(SaveSlotText2);
	SaveSlotTexts.Add(SaveSlotText3);
	SaveSlotTexts.Add(SaveSlotText4);

	// 장비 슬롯 위젯과 슬롯 타입 순서 캐시
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

	// 고정 4개 슬롯 버튼 바인딩
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
	// 고정 4개 슬롯 버튼 바인딩 정리
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

void UPRStartMenuWidget::RefreshSaveSlotButtons()
{
	for (int32 SlotArrayIndex = 0; SlotArrayIndex < SaveSlotButtons.Num(); ++SlotArrayIndex)
	{
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
}

void UPRStartMenuWidget::RefreshSelectedSavePreview(const FPRCharacterSaveData& SaveData)
{
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
			EquipmentSlotWidget->SetSlotViewData(BuildSavedEquipmentSlotViewData(SaveData, EquipmentSlotTypes[SlotIndex]));
		}
	}

	if (IsValid(CharacterPreviewWidget))
	{
		CharacterPreviewWidget->SetPreviewSaveData(SaveData);
	}
}

void UPRStartMenuWidget::SelectFirstAvailableSaveSlot()
{
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
}

void UPRStartMenuWidget::SelectSaveSlot(int32 SlotIndex)
{
	UPRGameInstance* GameInstance = GetProjectRGameInstance();
	if (!IsValid(GameInstance) || !GameInstance->DoesLocalCharacterSaveExist(SlotIndex))
	{
		return;
	}

	if (!GameInstance->LoadLocalCharacterSlot(SlotIndex))
	{
		return;
	}

	FPRCharacterSaveData SaveData;
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
	FPRStartMenuSaveSlotViewData ViewData;
	ViewData.SlotIndex = SlotIndex;
	ViewData.bSelected = SelectedSaveSlotIndex == SlotIndex;
	ViewData.DisplayName = FText::Format(FText::FromString(TEXT("슬롯 {0}")), FText::AsNumber(SlotIndex));

	FPRCharacterSaveData SaveData;
	const UPRGameInstance* GameInstance = GetProjectRGameInstance();
	if (IsValid(GameInstance) && GameInstance->TryGetLocalCharacterSaveSlotData(SlotIndex, SaveData))
	{
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
		return UPRInventoryItemSlotViewDataBuilder::BuildWeaponDataViewData(WeaponData, true);
	}

	return UPRInventoryItemSlotViewDataBuilder::BuildEmptyWeaponSlotViewData(SlotType);
}

FPRInventoryItemSlotViewData UPRStartMenuWidget::BuildSavedEquipmentSlotViewData(const FPRCharacterSaveData& SaveData, EPREquipmentSlotType SlotType) const
{
	if (UPREquipmentDataAsset* EquipmentData = ResolveSavedEquipmentData(SaveData, SlotType))
	{
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

		// 장비 저장 인덱스 기반 데이터 조회
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

	// 무기 저장 인덱스 기반 데이터 조회
	const FPRWeaponItemSaveEntry& WeaponEntry = SaveData.Inventory.Weapons[WeaponIndex];
	return WeaponEntry.WeaponData.LoadSynchronous();
}

UPRGameInstance* UPRStartMenuWidget::GetProjectRGameInstance() const
{
	return Cast<UPRGameInstance>(GetGameInstance());
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

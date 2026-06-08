// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/Game/PRGameTypes.h"
#include "ProjectR/ItemSystem/Types/PREquipmentTypes.h"
#include "ProjectR/UI/Inventory/PRInventoryUITypes.h"
#include "ProjectR/UI/PRWidgetBase.h"
#include "ProjectR/UI/StartMenu/PRStartMenuTypes.h"
#include "PRStartMenuWidget.generated.h"

class UButton;
class UEditableTextBox;
class UPRItemSlotWidget;
class UTextBlock;
class UPRCharacterPreviewWidget;
class UPRGameInstance;
class UPREquipmentDataAsset;
class UPRPlayerStatsPanelWidget;
class UPRWeaponDataAsset;

// 시작 메뉴에서 세이브 슬롯과 선택된 캐릭터 장착 상태를 표시하는 위젯
UCLASS(Abstract, BlueprintType)
class PROJECTR_API UPRStartMenuWidget : public UPRWidgetBase
{
	GENERATED_BODY()

protected:
	/*~ UUserWidget Interface ~*/
	// 화면 표시 시 세이브 슬롯과 장착 표시를 초기화함
	virtual void NativeConstruct() override;

	// 화면 제거 시 버튼 이벤트 바인딩을 정리함
	virtual void NativeDestruct() override;

private:
	// UMG BindWidget 포인터를 반복 처리용 배열로 캐싱함
	void CacheWidgetLists();

	// 세이브 슬롯 버튼 이벤트를 바인딩함
	void BindSaveSlotButtonEvents();

	// 세이브 슬롯 버튼 이벤트 바인딩을 해제함
	void UnbindSaveSlotButtonEvents();

	// 시작 버튼 이벤트 바인딩
	void BindStartButtonEvents();

	// 시작 버튼 이벤트 바인딩 해제
	void UnbindStartButtonEvents();

	// 세이브 삭제 버튼 이벤트 바인딩
	void BindDeleteSaveSlotButtonEvents();

	// 세이브 삭제 버튼 이벤트 바인딩 해제
	void UnbindDeleteSaveSlotButtonEvents();

	// 세션 상태 이벤트 바인딩
	void BindSessionEvents();

	// 세션 상태 이벤트 바인딩 해제
	void UnbindSessionEvents();

	// 세션 입력 기본값 설정
	void ConfigureSessionInputDefaults();

	// 세이브 슬롯 버튼과 라벨을 현재 파일 상태 기준으로 갱신함
	void RefreshSaveSlotButtons();

	// 시작 버튼 활성 상태 갱신
	void RefreshStartButtonEnabled(bool bEnabled);

	// 세이브 삭제 버튼 활성 상태 갱신
	void RefreshDeleteSaveSlotButtonEnabled();

	// 세션 상태 표시 텍스트 갱신
	void RefreshSessionStatusText(const FText& StatusText);

	// 선택된 세이브 데이터 기준으로 무기와 장비 슬롯을 갱신함
	void RefreshSelectedSavePreview(const FPRCharacterSaveData& SaveData);

	// 첫 번째 사용 가능한 세이브 슬롯을 선택함
	void SelectFirstAvailableSaveSlot();

	// 지정 세이브 슬롯을 선택함
	void SelectSaveSlot(int32 SlotIndex);

	// 세이브 슬롯 표시 데이터를 생성함
	FPRStartMenuSaveSlotViewData BuildSaveSlotViewData(int32 SlotIndex) const;

	// 세이브 슬롯 표시 텍스트를 반환함
	FText BuildSaveSlotDisplayText(const FPRStartMenuSaveSlotViewData& ViewData) const;

	// 저장 데이터의 무기 슬롯 표시 데이터를 생성함
	FPRInventoryItemSlotViewData BuildSavedWeaponSlotViewData(const FPRCharacterSaveData& SaveData, EPRWeaponSlotType SlotType) const;

	// 저장 데이터의 장비 슬롯 표시 데이터를 생성함
	FPRInventoryItemSlotViewData BuildSavedEquipmentSlotViewData(const FPRCharacterSaveData& SaveData, EPREquipmentSlotType SlotType) const;

	// 저장 데이터에서 지정 슬롯의 장비 데이터를 조회함
	UPREquipmentDataAsset* ResolveSavedEquipmentData(const FPRCharacterSaveData& SaveData, EPREquipmentSlotType SlotType) const;

	// 저장 데이터에서 지정 슬롯의 무기 데이터를 조회함
	UPRWeaponDataAsset* ResolveSavedWeaponData(const FPRCharacterSaveData& SaveData, EPRWeaponSlotType SlotType) const;

	// 현재 GameInstance를 ProjectR 타입으로 반환함
	UPRGameInstance* GetProjectRGameInstance() const;

	// 로컬 캐릭터 세이브가 하나 이상 존재하는지 확인함
	bool HasAnyLocalCharacterSave() const;

	// 시작 버튼 클릭 처리
	UFUNCTION()
	void HandleStartButtonClicked();

	// 선택된 세이브 슬롯 삭제 처리
	UFUNCTION()
	void HandleDeleteSaveSlotButtonClicked();

	// 세션 상태 변경 처리
	UFUNCTION()
	void HandleSessionStateChanged(EPRSessionState NewState);

	// 세션 실패 처리
	UFUNCTION()
	void HandleSessionFailed(EPRSessionFailReason Reason, FString Detail);

	// 1번 세이브 슬롯 클릭 처리
	UFUNCTION()
	void HandleSaveSlotButton1Clicked();

	// 2번 세이브 슬롯 클릭 처리
	UFUNCTION()
	void HandleSaveSlotButton2Clicked();

	// 3번 세이브 슬롯 클릭 처리
	UFUNCTION()
	void HandleSaveSlotButton3Clicked();

	// 4번 세이브 슬롯 클릭 처리
	UFUNCTION()
	void HandleSaveSlotButton4Clicked();

protected:
	// UMG에서 바인딩할 1번 세이브 슬롯 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "ProjectR|StartMenu")
	TObjectPtr<UButton> SaveSlotButton1;

	// UMG에서 바인딩할 2번 세이브 슬롯 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "ProjectR|StartMenu")
	TObjectPtr<UButton> SaveSlotButton2;

	// UMG에서 바인딩할 3번 세이브 슬롯 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "ProjectR|StartMenu")
	TObjectPtr<UButton> SaveSlotButton3;

	// UMG에서 바인딩할 4번 세이브 슬롯 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "ProjectR|StartMenu")
	TObjectPtr<UButton> SaveSlotButton4;

	// UMG에서 바인딩할 1번 세이브 슬롯 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|StartMenu")
	TObjectPtr<UTextBlock> SaveSlotText1;

	// UMG에서 바인딩할 2번 세이브 슬롯 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|StartMenu")
	TObjectPtr<UTextBlock> SaveSlotText2;

	// UMG에서 바인딩할 3번 세이브 슬롯 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|StartMenu")
	TObjectPtr<UTextBlock> SaveSlotText3;

	// UMG에서 바인딩할 4번 세이브 슬롯 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|StartMenu")
	TObjectPtr<UTextBlock> SaveSlotText4;

	// UMG에서 바인딩할 호스트 IP 입력창
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "ProjectR|StartMenu")
	TObjectPtr<UEditableTextBox> HostAddressTextBox;

	// UMG에서 바인딩할 세션 시작 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "ProjectR|StartMenu")
	TObjectPtr<UButton> StartButton;

	// UMG에서 바인딩할 선택 세이브 삭제 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|StartMenu")
	TObjectPtr<UButton> DeleteSaveSlotButton;

	// UMG에서 바인딩할 세션 상태 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|StartMenu")
	TObjectPtr<UTextBlock> SessionStatusText;

	// 호스트 시작 시 사용할 기본 맵
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|StartMenu")
	FName HostMapName = TEXT("L_Playground");

	// 호스트 시작 시 허용할 최대 인원
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|StartMenu", meta = (ClampMin = "1"))
	int32 HostMaxPlayers = 4;

	// UMG에서 바인딩할 주무기 슬롯 위젯
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|StartMenu")
	TObjectPtr<UPRItemSlotWidget> PrimaryWeaponSlotWidget;

	// UMG에서 바인딩할 보조무기 슬롯 위젯
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|StartMenu")
	TObjectPtr<UPRItemSlotWidget> SecondaryWeaponSlotWidget;

	// UMG에서 바인딩할 머리 장비 슬롯 위젯
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|StartMenu")
	TObjectPtr<UPRItemSlotWidget> HeadEquipmentSlotWidget;

	// UMG에서 바인딩할 몸통 장비 슬롯 위젯
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|StartMenu")
	TObjectPtr<UPRItemSlotWidget> BodyEquipmentSlotWidget;

	// UMG에서 바인딩할 손 장비 슬롯 위젯
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|StartMenu")
	TObjectPtr<UPRItemSlotWidget> HandsEquipmentSlotWidget;

	// UMG에서 바인딩할 다리 장비 슬롯 위젯
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|StartMenu")
	TObjectPtr<UPRItemSlotWidget> LegsEquipmentSlotWidget;

	// UMG에서 바인딩할 목걸이 장비 슬롯 위젯
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|StartMenu")
	TObjectPtr<UPRItemSlotWidget> AmuletEquipmentSlotWidget;

	// UMG에서 바인딩할 반지 1 장비 슬롯 위젯
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|StartMenu")
	TObjectPtr<UPRItemSlotWidget> Ring1EquipmentSlotWidget;

	// UMG에서 바인딩할 반지 2 장비 슬롯 위젯
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|StartMenu")
	TObjectPtr<UPRItemSlotWidget> Ring2EquipmentSlotWidget;

	// UMG에서 바인딩할 캐릭터 프리뷰 위젯
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "ProjectR|StartMenu")
	TObjectPtr<UPRCharacterPreviewWidget> CharacterPreviewWidget;

	// UMG에서 바인딩할 플레이어 스탯 패널 위젯
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|StartMenu")
	TObjectPtr<UPRPlayerStatsPanelWidget> PlayerStatsPanelWidget;

private:
	// 반복 처리용 세이브 슬롯 버튼 캐시
	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> SaveSlotButtons;

	// 반복 처리용 세이브 슬롯 텍스트 캐시
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> SaveSlotTexts;

	// 반복 처리용 장비 슬롯 위젯 캐시
	UPROPERTY(Transient)
	TArray<TObjectPtr<UPRItemSlotWidget>> EquipmentSlotWidgets;

	// 장비 슬롯 위젯 캐시와 같은 인덱스를 사용하는 슬롯 타입 캐시
	UPROPERTY(Transient)
	TArray<EPREquipmentSlotType> EquipmentSlotTypes;

	// 현재 선택된 세이브 슬롯 번호
	int32 SelectedSaveSlotIndex = INDEX_NONE;
};

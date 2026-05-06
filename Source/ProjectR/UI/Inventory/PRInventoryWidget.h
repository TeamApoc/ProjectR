// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/Inventory/Components/PRInventoryComponent.h"
#include "ProjectR/UI/Inventory/PRInventoryUITypes.h"
#include "ProjectR/Weapon/Types/PRWeaponTypes.h"
#include "ProjectR/UI/PRWidgetBase.h"
#include "PRInventoryWidget.generated.h"

class UPRItemInstance_Mod;
class UPRItemDataAsset;
class UPRInventoryComponent;
class UPRInventoryItemListWidget;
class UPRItemInstance_Weapon;
class UPRItemSlotWidget;
class UPRWeaponManagerComponent;

// 인벤토리 화면의 무기 슬롯과 아이템 선택 목록을 연결하는 최상위 위젯이다
UCLASS(Abstract, BlueprintType)
class PROJECTR_API UPRInventoryWidget : public UPRWidgetBase
{
	GENERATED_BODY()

public:
	// 인벤토리 위젯에 표시할 아이템 소스 보유 컴포넌트(인벤토리, 무기 매니저)를 설정한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Inventory")
	void SetInventorySources(UPRInventoryComponent* InInventoryComponent, UPRWeaponManagerComponent* InWeaponManagerComponent);

	// ============ Getter =============
	// 현재 인벤토리 컴포넌트를 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|Inventory")
	UPRInventoryComponent* GetInventoryComponent() const {return InventoryComponent;}

	// 현재 무기 매니저 컴포넌트를 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|Inventory")
	UPRWeaponManagerComponent* GetWeaponManagerComponent() const {return WeaponManagerComponent;}

	// 무기 리스트 대상 슬롯을 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|Inventory")
	EPRWeaponSlotType GetPendingWeaponListSlot() const {return PendingWeaponListSlot;}

	// Mod 리스트 대상 무기를 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|Inventory")
	UPRItemInstance_Weapon* GetPendingModTargetWeapon() const {return PendingModTargetWeaponItem;}
	// =========================
protected:
	/*~ UUserWidget Interface ~*/
	// 화면에 표시될 때 하위 위젯 이벤트를 바인딩하고 표시 상태를 초기화한다
	virtual void NativeConstruct() override;

	// 화면에서 제거될 때 표시 상태와 하위 위젯 이벤트 바인딩을 정리한다
	virtual void NativeDestruct() override;

private:
	// 하위 슬롯과 리스트 이벤트를 바인딩한다
	void BindChildWidgetEvents();

	// 하위 슬롯과 리스트 이벤트 바인딩을 정리한다
	void UnbindChildWidgetEvents();

	// 인벤토리와 무기 매니저 변경 이벤트를 바인딩한다
	void BindInventorySourceEvents();

	// 인벤토리와 무기 매니저 변경 이벤트 바인딩을 정리한다
	void UnbindInventorySourceEvents();

	// ========= 이벤트 바인드 처리 함수 =========
	// 주무기 슬롯 좌클릭을 처리한다
	UFUNCTION()
	void HandlePrimaryWeaponSlotLeftClicked(const FPRInventoryItemSlotViewData& ViewData);

	// 주무기 슬롯 우클릭을 처리한다
	UFUNCTION()
	void HandlePrimaryWeaponSlotRightClicked(const FPRInventoryItemSlotViewData& ViewData);

	// 보조무기 슬롯 좌클릭을 처리한다
	UFUNCTION()
	void HandleSecondaryWeaponSlotLeftClicked(const FPRInventoryItemSlotViewData& ViewData);

	// 보조무기 슬롯 우클릭을 처리한다
	UFUNCTION()
	void HandleSecondaryWeaponSlotRightClicked(const FPRInventoryItemSlotViewData& ViewData);

	// 리스트에서 선택한 아이템을 장착 요청으로 변환한다
	UFUNCTION()
	void HandleItemListSelection(const FPRInventoryItemSlotViewData& ViewData);

	// 인벤토리 변경 알림을 받아 화면을 갱신한다
	UFUNCTION()
	void HandleInventoryChanged(UPRInventoryComponent* ChangedInventoryComponent, EPRInventoryChangeReason ChangeReason);

	// 무기 장착 변경 알림을 받아 화면을 갱신한다
	UFUNCTION()
	void HandleWeaponEquipmentChanged(UPRWeaponManagerComponent* ChangedWeaponManagerComponent, EPRWeaponSlotType ChangedSlot);
	// =======================


	// 지정 슬롯의 무기 목록을 연다
	void OpenWeaponList(EPRWeaponSlotType TargetSlot);

	// 지정 무기의 Mod 목록을 연다
	void OpenModList(UPRItemInstance_Weapon* TargetWeaponItem);

	// 아이템 리스트를 숨긴다
	void CloseItemList();

	// 현재 장착 슬롯 위젯을 갱신한다
	void RefreshEquippedSlotWidgets();

	// 장착 슬롯과 열린 리스트를 현재 데이터 기준으로 갱신한다
	void RefreshInventoryView();

	// 무기 슬롯 뷰 데이터를 만든다
	FPRInventoryItemSlotViewData BuildWeaponSlotViewData(EPRWeaponSlotType SlotType) const;

	// 무기 아이템 뷰 데이터를 만든다
	FPRInventoryItemSlotViewData BuildWeaponItemViewData(UPRItemInstance_Weapon* WeaponItem, bool bEquipped) const;

	// Mod 아이템 뷰 데이터를 만든다
	FPRInventoryItemSlotViewData BuildModItemViewData(UPRItemInstance_Mod* ModItem, bool bEquipped) const;

	// 장착 해제 항목 뷰 데이터를 만든다
	FPRInventoryItemSlotViewData BuildUnequipViewData(EPRItemType ListType) const;

	// 지정 무기에 Mod가 장착 가능한지 확인한다
	bool IsModCompatibleWithWeapon(const UPRItemInstance_Mod* ModItem, const UPRItemInstance_Weapon* WeaponItem) const;

protected:
	// UMG에서 바인딩할 주무기 슬롯 위젯
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Inventory")
	TObjectPtr<UPRItemSlotWidget> PrimaryWeaponSlotWidget;

	// UMG에서 바인딩할 보조무기 슬롯 위젯
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Inventory")
	TObjectPtr<UPRItemSlotWidget> SecondaryWeaponSlotWidget;

	// UMG에서 바인딩할 아이템 리스트 위젯
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Inventory")
	TObjectPtr<UPRInventoryItemListWidget> ItemListWidget;

private:
	// 아이템 보유 인벤토리 컴포넌트
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Inventory", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPRInventoryComponent> InventoryComponent;

	// 장착 무기 보유 무기 매니저 컴포넌트
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Inventory", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPRWeaponManagerComponent> WeaponManagerComponent;

	// 무기 리스트 팝업 위젯에 띄울 아이템의 무기 슬롯
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Inventory", meta = (AllowPrivateAccess = "true"))
	EPRWeaponSlotType PendingWeaponListSlot = EPRWeaponSlotType::None;

	// Mod 장착 타겟 무기 Item
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Inventory", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPRItemInstance_Weapon> PendingModTargetWeaponItem;
};

// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRInventoryWidget.h"

#include "ProjectR/Inventory/Components/PRInventoryComponent.h"
#include "ProjectR/Inventory/Data/PRItemDataAsset.h"
#include "ProjectR/Weapon/Components/PRWeaponManagerComponent.h"
#include "ProjectR/Weapon/Data/PRWeaponDataAsset.h"
#include "ProjectR/Weapon/Data/PRWeaponModDataAsset.h"
#include "ProjectR/Weapon/Items/PRItemInstance_Mod.h"
#include "ProjectR/Weapon/Items/PRItemInstance_Weapon.h"
#include "ProjectR/UI/Inventory/PRInventoryItemListWidget.h"
#include "ProjectR/UI/Inventory/PRItemSlotWidget.h"

void UPRInventoryWidget::SetInventorySources(UPRInventoryComponent* InInventoryComponent, UPRWeaponManagerComponent* InWeaponManagerComponent)
{
	InventoryComponent = InInventoryComponent;
	WeaponManagerComponent = InWeaponManagerComponent;

	// 아이템 장착 슬롯 갱신
	RefreshEquippedSlotWidgets();
}

/*~ UUserWidget Interface ~*/

void UPRInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 하위 슬롯 이벤트 바인드
	BindChildWidgetEvents();
	// 아이템 리스트 숨기기
	CloseItemList();
	// 아이템 장착 슬롯 갱신
	RefreshEquippedSlotWidgets();
}

void UPRInventoryWidget::NativeDestruct()
{
	// 아이템 리스트 숨기기
	CloseItemList();
	// 하위 위젯 이벤트 바인딩 정리
	UnbindChildWidgetEvents();

	Super::NativeDestruct();
}

void UPRInventoryWidget::BindChildWidgetEvents()
{
	// 이벤트 바인드 전 하위 위젯 이벤트 초기화
	UnbindChildWidgetEvents();

	// 주무기 슬롯 위젯 마우스 클릭 이벤트 바인드
	if (IsValid(PrimaryWeaponSlotWidget))
	{
		PrimaryWeaponSlotWidget->OnLeftClicked.AddDynamic(this, &UPRInventoryWidget::HandlePrimaryWeaponSlotLeftClicked);
		PrimaryWeaponSlotWidget->OnRightClicked.AddDynamic(this, &UPRInventoryWidget::HandlePrimaryWeaponSlotRightClicked);
	}

	// 보조무기 슬롯 위젯 마우스 클릭 이벤트 바인드
	if (IsValid(SecondaryWeaponSlotWidget))
	{
		SecondaryWeaponSlotWidget->OnLeftClicked.AddDynamic(this, &UPRInventoryWidget::HandleSecondaryWeaponSlotLeftClicked);
		SecondaryWeaponSlotWidget->OnRightClicked.AddDynamic(this, &UPRInventoryWidget::HandleSecondaryWeaponSlotRightClicked);
	}

	// 아이템 리스트 위젯 이벤트 바인드
	if (IsValid(ItemListWidget))
	{
		ItemListWidget->OnItemSelected.AddDynamic(this, &UPRInventoryWidget::HandleItemListSelection);
	}
}

void UPRInventoryWidget::UnbindChildWidgetEvents()
{
	// 주무기 슬롯 이벤트 언바인드
	if (IsValid(PrimaryWeaponSlotWidget))
	{
		PrimaryWeaponSlotWidget->OnLeftClicked.RemoveDynamic(this, &UPRInventoryWidget::HandlePrimaryWeaponSlotLeftClicked);
		PrimaryWeaponSlotWidget->OnRightClicked.RemoveDynamic(this, &UPRInventoryWidget::HandlePrimaryWeaponSlotRightClicked);
	}

	// 보조무기 슬롯 이벤트 언바인드
	if (IsValid(SecondaryWeaponSlotWidget))
	{
		SecondaryWeaponSlotWidget->OnLeftClicked.RemoveDynamic(this, &UPRInventoryWidget::HandleSecondaryWeaponSlotLeftClicked);
		SecondaryWeaponSlotWidget->OnRightClicked.RemoveDynamic(this, &UPRInventoryWidget::HandleSecondaryWeaponSlotRightClicked);
	}

	// 아이템 리스트 이벤트 언바인드
	if (IsValid(ItemListWidget))
	{
		ItemListWidget->OnItemSelected.RemoveDynamic(this, &UPRInventoryWidget::HandleItemListSelection);
	}
}

//
void UPRInventoryWidget::HandlePrimaryWeaponSlotLeftClicked(const FPRInventoryItemSlotViewData& ViewData)
{
	// 주무기 슬롯 위젯 좌클릭 시 주무기 아이템 리스트 열기
	OpenWeaponList(EPRWeaponSlotType::Primary);
}

void UPRInventoryWidget::HandlePrimaryWeaponSlotRightClicked(const FPRInventoryItemSlotViewData& ViewData)
{
	// 주무기 슬롯 위젯 우클릭 시 장착 가능한 모드 리스트 열기
	UPRItemInstance_Weapon* WeaponItem = IsValid(WeaponManagerComponent)
		? WeaponManagerComponent->GetWeaponInstanceBySlotType(EPRWeaponSlotType::Primary)
		: nullptr;

	OpenModList(WeaponItem);
}

void UPRInventoryWidget::HandleSecondaryWeaponSlotLeftClicked(const FPRInventoryItemSlotViewData& ViewData)
{
	// 보조무기 슬롯 위젯 좌클릭 시 주무기 아이템 리스트 열기
	OpenWeaponList(EPRWeaponSlotType::Secondary);
}

void UPRInventoryWidget::HandleSecondaryWeaponSlotRightClicked(const FPRInventoryItemSlotViewData& ViewData)
{
	// 보조무기 슬롯 위젯 우클릭 시 장착 가능한 모드 리스트 열기
	UPRItemInstance_Weapon* WeaponItem = IsValid(WeaponManagerComponent)
		? WeaponManagerComponent->GetWeaponInstanceBySlotType(EPRWeaponSlotType::Secondary)
		: nullptr;

	OpenModList(WeaponItem);
}

void UPRInventoryWidget::HandleItemListSelection(const FPRInventoryItemSlotViewData& ViewData)
{
	if (!IsValid(ItemListWidget))
	{
		return;
	}

	const EPRInventoryItemListType CurrentListType = ItemListWidget->GetListType();
	if (CurrentListType == EPRInventoryItemListType::Weapon)
	{
		if (!IsValid(WeaponManagerComponent))
		{
			return;
		}

		if (ViewData.bUnequipEntry)
		{
			WeaponManagerComponent->UnequipWeaponFromSlot(PendingWeaponListSlot);
		}
		else if (UPRItemInstance_Weapon* WeaponItem = Cast<UPRItemInstance_Weapon>(ViewData.ItemInstance.Get()))
		{
			WeaponManagerComponent->EquipWeapon(WeaponItem);
		}
	}
	else if (CurrentListType == EPRInventoryItemListType::Mod)
	{
		if (!IsValid(InventoryComponent) || !IsValid(PendingModTargetWeaponItem))
		{
			return;
		}

		if (ViewData.bUnequipEntry)
		{
			InventoryComponent->RequestUnequipModFromWeapon(PendingModTargetWeaponItem);
		}
		else if (UPRItemInstance_Mod* ModItem = Cast<UPRItemInstance_Mod>(ViewData.ItemInstance.Get()))
		{
			InventoryComponent->RequestEquipModItemToWeapon(ModItem, PendingModTargetWeaponItem);
		}
	}

	CloseItemList();
	// 아이템 장착 슬롯 갱신
	RefreshEquippedSlotWidgets();
}

void UPRInventoryWidget::OpenWeaponList(EPRWeaponSlotType TargetSlot)
{
	// 유효성 확인. 아이템 리스트 위젯, 인벤토리 컴포넌트, 무기매니저 컴포넌트
	if (!IsValid(ItemListWidget) || !IsValid(InventoryComponent) || !IsValid(WeaponManagerComponent))
	{
		UE_LOG(LogTemp, Warning, TEXT("[InventoryWidget] OpenWeaponList() 실행 조건이 유효하지 않습니다."));
		return;
	}

	// 무기 리스트 팝업 위젯에 띄울 아이템의 무기 슬롯
	PendingWeaponListSlot = TargetSlot;
	// Mod 장착 타겟 무기 Item
	PendingModTargetWeaponItem = nullptr;

	TArray<FPRInventoryItemSlotViewData> ListItems;
	if (IsValid(WeaponManagerComponent->GetWeaponInstanceBySlotType(TargetSlot)))
	{
		ListItems.Add(BuildUnequipViewData(EPRInventoryItemListType::Weapon));
	}

	for (UPRItemInstance_Weapon* WeaponItem : InventoryComponent->InventoryWeaponItems)
	{
		if (!IsValid(WeaponItem) || !IsValid(WeaponItem->GetWeaponData()))
		{
			continue;
		}

		if (WeaponItem->GetWeaponData()->SlotType != TargetSlot)
		{
			continue;
		}

		const bool bEquipped = WeaponManagerComponent->GetWeaponInstanceBySlotType(TargetSlot) == WeaponItem;
		ListItems.Add(BuildWeaponItemViewData(WeaponItem, bEquipped));
	}

	ItemListWidget->SetItemList(EPRInventoryItemListType::Weapon, ListItems);
	ItemListWidget->SetVisibility(ESlateVisibility::Visible);
}

void UPRInventoryWidget::OpenModList(UPRItemInstance_Weapon* TargetWeaponItem)
{
	// 아이템 리스트 위젯, 인벤토리 컴포넌트, 장착 타겟 무기 아이템 유효성 확인
	if (!IsValid(ItemListWidget) || !IsValid(InventoryComponent) || !IsValid(TargetWeaponItem))
	{
		UE_LOG(LogTemp, Warning, TEXT("[InventoryWidget] 모드 리스트 열기 실패. OpenModList()"));
		return;
	}

	PendingWeaponListSlot = EPRWeaponSlotType::None;
	PendingModTargetWeaponItem = TargetWeaponItem;

	TArray<FPRInventoryItemSlotViewData> ListItems;
	if (IsValid(TargetWeaponItem->GetEquippedModItem()) || IsValid(TargetWeaponItem->GetModData()))
	{
		ListItems.Add(BuildUnequipViewData(EPRInventoryItemListType::Mod));
	}

	for (UPRItemInstance_Mod* ModItem : InventoryComponent->InventoryModItems)
	{
		if (!IsModCompatibleWithWeapon(ModItem, TargetWeaponItem))
		{
			continue;
		}

		const bool bEquipped = TargetWeaponItem->GetEquippedModItem() == ModItem;
		ListItems.Add(BuildModItemViewData(ModItem, bEquipped));
	}

	ItemListWidget->SetItemList(EPRInventoryItemListType::Mod, ListItems);
	ItemListWidget->SetVisibility(ESlateVisibility::Visible);
}

void UPRInventoryWidget::CloseItemList()
{
	//아이템 리스트 위젯이 유효하면
	if (IsValid(ItemListWidget))
	{
		// 빈 아이템 뷰 데이터 목록 설정
		TArray<FPRInventoryItemSlotViewData> EmptyItems;
		ItemListWidget->SetItemList(EPRInventoryItemListType::None, EmptyItems);
		ItemListWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	PendingWeaponListSlot = EPRWeaponSlotType::None;
	PendingModTargetWeaponItem = nullptr;
}

void UPRInventoryWidget::RefreshEquippedSlotWidgets()
{
	// 주무기 슬롯 위젯이 유효하면
	if (IsValid(PrimaryWeaponSlotWidget))
	{
		//주무기 슬롯 갱신
		PrimaryWeaponSlotWidget->SetSlotViewData(BuildWeaponSlotViewData(EPRWeaponSlotType::Primary));
	}

	// 보조무기 슬롯 위젯이 유효하면
	if (IsValid(SecondaryWeaponSlotWidget))
	{
		// 보조무기 슬롯 갱신
		SecondaryWeaponSlotWidget->SetSlotViewData(BuildWeaponSlotViewData(EPRWeaponSlotType::Secondary));
	}
}

FPRInventoryItemSlotViewData UPRInventoryWidget::BuildWeaponSlotViewData(EPRWeaponSlotType SlotType) const
{
	//무기 매니저 컴포넌트가 유효하면
	if (IsValid(WeaponManagerComponent))
	{
		// 타겟 슬롯의 장착된 무기 아이템 확인
		if (UPRItemInstance_Weapon* WeaponItem = WeaponManagerComponent->GetWeaponInstanceBySlotType(SlotType))
		{
			// 장착 아이템 뷰 데이터 리턴
			return BuildWeaponItemViewData(WeaponItem, true);
		}
	}

	// 장착된 무기가 없으면 슬롯 텍스트에 무기 없음으로 표시
	FPRInventoryItemSlotViewData ViewData;
	ViewData.ItemType = EPRItemType::Weapon;
	ViewData.DisplayName = SlotType == EPRWeaponSlotType::Primary
		? FText::FromString(TEXT("주무기 없음"))
		: FText::FromString(TEXT("보조무기 없음"));
	return ViewData;
}

FPRInventoryItemSlotViewData UPRInventoryWidget::BuildWeaponItemViewData(UPRItemInstance_Weapon* WeaponItem, bool bEquipped) const
{
	// 무기 아이템 표시용 뷰 데이터
	FPRInventoryItemSlotViewData ViewData;
	// 무기 아이템이 없으면
	if (!IsValid(WeaponItem))
	{
		UE_LOG(LogTemp, Warning, TEXT("[InventoryWidget] 무기 아이템 없음. BuildWeaponItemViewData()"));
		// 빈 뷰 데이터 리턴
		return ViewData;
	}

	//
	UPRWeaponDataAsset* WeaponData = WeaponItem->GetWeaponData();
	ViewData.ItemData = WeaponData;
	ViewData.ItemInstance = WeaponItem;
	ViewData.ItemType = EPRItemType::Weapon;
	ViewData.bEquipped = bEquipped;

	if (IsValid(WeaponData))
	{
		UE_LOG(LogTemp, Warning, TEXT("[InventoryWidget] 무기 아이템 데이터 없음. BuildWeaponItemViewData()"));
		ViewData.DisplayName = WeaponData->GetDisplayName();
		ViewData.Icon = WeaponData->GetIcon();
	}

	return ViewData;
}

FPRInventoryItemSlotViewData UPRInventoryWidget::BuildModItemViewData(UPRItemInstance_Mod* ModItem, bool bEquipped) const
{
	FPRInventoryItemSlotViewData ViewData;
	if (!IsValid(ModItem))
	{
		UE_LOG(LogTemp, Warning, TEXT("[InventoryWidget] 모드 아이템 없음. BuildModItemViewData()"));
		return ViewData;
	}

	UPRWeaponModDataAsset* ModData = ModItem->GetModData();
	ViewData.ItemData = ModData;
	ViewData.ItemInstance = ModItem;
	ViewData.ItemType = EPRItemType::Mod;
	ViewData.bEquipped = bEquipped;

	if (IsValid(ModData))
	{
		ViewData.DisplayName = ModData->GetDisplayName();
		ViewData.Icon = ModData->GetIcon();
	}

	return ViewData;
}

FPRInventoryItemSlotViewData UPRInventoryWidget::BuildUnequipViewData(EPRInventoryItemListType ListType) const
{
	FPRInventoryItemSlotViewData ViewData;
	ViewData.bUnequipEntry = true;

	if (ListType == EPRInventoryItemListType::Weapon)
	{
		ViewData.ItemType = EPRItemType::Weapon;
		ViewData.DisplayName = FText::FromString(TEXT("무기 해제"));
	}
	else if (ListType == EPRInventoryItemListType::Mod)
	{
		ViewData.ItemType = EPRItemType::Mod;
		ViewData.DisplayName = FText::FromString(TEXT("Mod 해제"));
	}

	return ViewData;
}

bool UPRInventoryWidget::IsModCompatibleWithWeapon(const UPRItemInstance_Mod* ModItem, const UPRItemInstance_Weapon* WeaponItem) const
{
	if (!IsValid(ModItem) || !IsValid(WeaponItem))
	{
		return false;
	}

	const UPRWeaponModDataAsset* ModData = ModItem->GetModData();
	const UPRWeaponDataAsset* WeaponData = WeaponItem->GetWeaponData();
	if (!IsValid(ModData) || !IsValid(WeaponData))
	{
		return false;
	}

	if (ModData->ModTags.IsEmpty())
	{
		return true;
	}

	if (WeaponData->SupportedModTags.IsEmpty())
	{
		return false;
	}

	return WeaponData->SupportedModTags.HasAny(ModData->ModTags);
}

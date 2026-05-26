// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRInventoryWidget.h"

#include "GameFramework/PlayerController.h"
#include "Components/TextBlock.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/ItemSystem/Components/PRInventoryComponent.h"
#include "ProjectR/ItemSystem/Data/PRConsumableDataAsset.h"
#include "ProjectR/ItemSystem/Data/PRItemDataAsset.h"
#include "ProjectR/ItemSystem/Data/PRMaterialDataAsset.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Consumable.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Material.h"
#include "ProjectR/ItemSystem/Components/PRQuickSlotComponent.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "ProjectR/Player/Components/PRCurrencyComponent.h"
#include "ProjectR/UI/Inventory/PRCurrencyDisplayWidget.h"
#include "ProjectR/ItemSystem/Components/PRWeaponManagerComponent.h"
#include "ProjectR/ItemSystem/Data/PRWeaponDataAsset.h"
#include "ProjectR/ItemSystem/Data/PRWeaponModDataAsset.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Mod.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Weapon.h"
#include "ProjectR/UI/Inventory/PRInventoryItemListWidget.h"
#include "ProjectR/UI/Inventory/PRItemSlotWidget.h"
#include "ProjectR/UI/Preview/PRCharacterPreviewWidget.h"

void UPRInventoryWidget::SetInventorySources(UPRInventoryComponent* InInventoryComponent, UPRWeaponManagerComponent* InWeaponManagerComponent, UPRQuickSlotComponent* InQuickSlotComponent)
{
	UnbindInventorySourceEvents();

	InventoryComponent = InInventoryComponent;
	WeaponManagerComponent = InWeaponManagerComponent;
	QuickSlotComponent = InQuickSlotComponent;

	BindInventorySourceEvents();
	RefreshInventoryView();
	RefreshCharacterPreviewWidget();
}

/*~ UUserWidget Interface ~*/

void UPRInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 하위 슬롯 이벤트 바인드
	BindChildWidgetEvents();
	// 인벤토리 소스 이벤트 바인드
	BindInventorySourceEvents();
	// 아이템 리스트 숨기기
	CloseItemList();
	// 인벤토리 화면 갱신
	RefreshInventoryView();
	// 캐릭터 프리뷰 갱신
	RefreshCharacterPreviewWidget();
}

void UPRInventoryWidget::NativeDestruct()
{
	// 아이템 리스트 숨기기
	CloseItemList();
	// 하위 위젯 이벤트 바인딩 정리
	UnbindChildWidgetEvents();
	// 인벤토리 소스 이벤트 바인딩 정리
	UnbindInventorySourceEvents();

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

	if (IsValid(QuickSlotItemSlotWidget0))
	{
		QuickSlotItemSlotWidget0->OnLeftClicked.AddDynamic(this, &UPRInventoryWidget::HandleQuickSlot0LeftClicked);
	}

	if (IsValid(QuickSlotItemSlotWidget1))
	{
		QuickSlotItemSlotWidget1->OnLeftClicked.AddDynamic(this, &UPRInventoryWidget::HandleQuickSlot1LeftClicked);
	}

	if (IsValid(QuickSlotItemSlotWidget2))
	{
		QuickSlotItemSlotWidget2->OnLeftClicked.AddDynamic(this, &UPRInventoryWidget::HandleQuickSlot2LeftClicked);
	}

	if (IsValid(QuickSlotItemSlotWidget3))
	{
		QuickSlotItemSlotWidget3->OnLeftClicked.AddDynamic(this, &UPRInventoryWidget::HandleQuickSlot3LeftClicked);
	}

	if (IsValid(MaterialSlotWidget))
	{
		MaterialSlotWidget->OnLeftClicked.AddDynamic(this, &UPRInventoryWidget::HandleMaterialSlotLeftClicked);
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

	if (IsValid(QuickSlotItemSlotWidget0))
	{
		QuickSlotItemSlotWidget0->OnLeftClicked.RemoveDynamic(this, &UPRInventoryWidget::HandleQuickSlot0LeftClicked);
	}

	if (IsValid(QuickSlotItemSlotWidget1))
	{
		QuickSlotItemSlotWidget1->OnLeftClicked.RemoveDynamic(this, &UPRInventoryWidget::HandleQuickSlot1LeftClicked);
	}

	if (IsValid(QuickSlotItemSlotWidget2))
	{
		QuickSlotItemSlotWidget2->OnLeftClicked.RemoveDynamic(this, &UPRInventoryWidget::HandleQuickSlot2LeftClicked);
	}

	if (IsValid(QuickSlotItemSlotWidget3))
	{
		QuickSlotItemSlotWidget3->OnLeftClicked.RemoveDynamic(this, &UPRInventoryWidget::HandleQuickSlot3LeftClicked);
	}

	if (IsValid(MaterialSlotWidget))
	{
		MaterialSlotWidget->OnLeftClicked.RemoveDynamic(this, &UPRInventoryWidget::HandleMaterialSlotLeftClicked);
	}

	// 아이템 리스트 이벤트 언바인드
	if (IsValid(ItemListWidget))
	{
		ItemListWidget->OnItemSelected.RemoveDynamic(this, &UPRInventoryWidget::HandleItemListSelection);
	}
}

void UPRInventoryWidget::BindInventorySourceEvents()
{
	if (IsValid(InventoryComponent))
	{
		InventoryComponent->GetOnInventoryChanged().RemoveDynamic(this, &UPRInventoryWidget::HandleInventoryChanged);
		InventoryComponent->GetOnInventoryChanged().AddDynamic(this, &UPRInventoryWidget::HandleInventoryChanged);
	}

	if (IsValid(WeaponManagerComponent))
	{
		WeaponManagerComponent->GetOnWeaponEquipmentChanged().RemoveDynamic(this, &UPRInventoryWidget::HandleWeaponEquipmentChanged);
		WeaponManagerComponent->GetOnWeaponEquipmentChanged().AddDynamic(this, &UPRInventoryWidget::HandleWeaponEquipmentChanged);
	}

	if (IsValid(QuickSlotComponent))
	{
		QuickSlotComponent->GetOnQuickSlotChanged().RemoveDynamic(this, &UPRInventoryWidget::HandleQuickSlotChanged);
		QuickSlotComponent->GetOnQuickSlotChanged().AddDynamic(this, &UPRInventoryWidget::HandleQuickSlotChanged);
	}

	CurrencyComponent = ResolveCurrencyComponent();
	if (IsValid(CurrencyComponent))
	{
		CurrencyComponent->OnScrapChanged.RemoveDynamic(this, &UPRInventoryWidget::HandleScrapChanged);
		CurrencyComponent->OnScrapChanged.AddDynamic(this, &UPRInventoryWidget::HandleScrapChanged);
	}
}

void UPRInventoryWidget::UnbindInventorySourceEvents()
{
	if (IsValid(InventoryComponent))
	{
		InventoryComponent->GetOnInventoryChanged().RemoveDynamic(this, &UPRInventoryWidget::HandleInventoryChanged);
	}

	if (IsValid(WeaponManagerComponent))
	{
		WeaponManagerComponent->GetOnWeaponEquipmentChanged().RemoveDynamic(this, &UPRInventoryWidget::HandleWeaponEquipmentChanged);
	}

	if (IsValid(QuickSlotComponent))
	{
		QuickSlotComponent->GetOnQuickSlotChanged().RemoveDynamic(this, &UPRInventoryWidget::HandleQuickSlotChanged);
	}

	if (IsValid(CurrencyComponent))
	{
		CurrencyComponent->OnScrapChanged.RemoveDynamic(this, &UPRInventoryWidget::HandleScrapChanged);
	}

	CurrencyComponent = nullptr;
}

//
void UPRInventoryWidget::HandlePrimaryWeaponSlotLeftClicked(const FPRInventoryItemSlotViewData& ViewData)
{
	if (IsItemListOpenAs(EPRItemType::PrimaryWeapon) && PendingWeaponListSlot == EPRWeaponSlotType::Primary)
	{
		CloseItemList();
		return;
	}

	// 주무기 슬롯 위젯 좌클릭 시 주무기 아이템 리스트 열기
	OpenWeaponList(EPRWeaponSlotType::Primary);
}

void UPRInventoryWidget::HandlePrimaryWeaponSlotRightClicked(const FPRInventoryItemSlotViewData& ViewData)
{
	// 주무기 슬롯 위젯 우클릭 시 장착 가능한 모드 리스트 열기
	UPRItemInstance_Weapon* WeaponItem = IsValid(WeaponManagerComponent)
		? WeaponManagerComponent->GetWeaponInstanceBySlotType(EPRWeaponSlotType::Primary)
		: nullptr;

	if (IsItemListOpenAs(EPRItemType::Mod) && PendingModTargetWeaponItem == WeaponItem)
	{
		CloseItemList();
		return;
	}

	OpenModList(WeaponItem);
}

void UPRInventoryWidget::HandleSecondaryWeaponSlotLeftClicked(const FPRInventoryItemSlotViewData& ViewData)
{
	if (IsItemListOpenAs(EPRItemType::SecondaryWeapon) && PendingWeaponListSlot == EPRWeaponSlotType::Secondary)
	{
		CloseItemList();
		return;
	}

	// 보조무기 슬롯 위젯 좌클릭 시 주무기 아이템 리스트 열기
	OpenWeaponList(EPRWeaponSlotType::Secondary);
}

void UPRInventoryWidget::HandleSecondaryWeaponSlotRightClicked(const FPRInventoryItemSlotViewData& ViewData)
{
	// 보조무기 슬롯 위젯 우클릭 시 장착 가능한 모드 리스트 열기
	UPRItemInstance_Weapon* WeaponItem = IsValid(WeaponManagerComponent)
		? WeaponManagerComponent->GetWeaponInstanceBySlotType(EPRWeaponSlotType::Secondary)
		: nullptr;

	if (IsItemListOpenAs(EPRItemType::Mod) && PendingModTargetWeaponItem == WeaponItem)
	{
		CloseItemList();
		return;
	}

	OpenModList(WeaponItem);
}

void UPRInventoryWidget::HandleQuickSlot0LeftClicked(const FPRInventoryItemSlotViewData& ViewData)
{
	if (IsItemListOpenAs(EPRItemType::Consumable) && PendingQuickSlotIndex == 0)
	{
		CloseItemList();
		return;
	}

	OpenConsumableListForQuickSlot(0);
}

void UPRInventoryWidget::HandleQuickSlot1LeftClicked(const FPRInventoryItemSlotViewData& ViewData)
{
	if (IsItemListOpenAs(EPRItemType::Consumable) && PendingQuickSlotIndex == 1)
	{
		CloseItemList();
		return;
	}

	OpenConsumableListForQuickSlot(1);
}

void UPRInventoryWidget::HandleQuickSlot2LeftClicked(const FPRInventoryItemSlotViewData& ViewData)
{
	if (IsItemListOpenAs(EPRItemType::Consumable) && PendingQuickSlotIndex == 2)
	{
		CloseItemList();
		return;
	}

	OpenConsumableListForQuickSlot(2);
}

void UPRInventoryWidget::HandleQuickSlot3LeftClicked(const FPRInventoryItemSlotViewData& ViewData)
{
	if (IsItemListOpenAs(EPRItemType::Consumable) && PendingQuickSlotIndex == 3)
	{
		CloseItemList();
		return;
	}

	OpenConsumableListForQuickSlot(3);
}

void UPRInventoryWidget::HandleMaterialSlotLeftClicked(const FPRInventoryItemSlotViewData& ViewData)
{
	if (IsItemListOpenAs(EPRItemType::Material))
	{
		CloseItemList();
		return;
	}

	OpenMaterialList();
}

void UPRInventoryWidget::HandleItemListSelection(const FPRInventoryItemSlotViewData& ViewData)
{
	if (!IsValid(ItemListWidget))
	{
		return;
	}

	const EPRItemType CurrentListType = ItemListWidget->GetListType();
	if (CurrentListType == EPRItemType::Weapon
		|| CurrentListType == EPRItemType::PrimaryWeapon
		|| CurrentListType == EPRItemType::SecondaryWeapon)
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
	else if (CurrentListType == EPRItemType::Mod)
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
	else if (CurrentListType == EPRItemType::Consumable)
	{
		if (!IsValid(QuickSlotComponent) || PendingQuickSlotIndex == INDEX_NONE)
		{
			return;
		}

		UPRConsumableDataAsset* ConsumableData = Cast<UPRConsumableDataAsset>(ViewData.ItemData.Get());
		if (IsValid(ConsumableData))
		{
			QuickSlotComponent->RequestRegisterQuickSlotItem(PendingQuickSlotIndex, ConsumableData);
		}
	}
	else if (CurrentListType == EPRItemType::Material)
	{
		// 재료는 현재 인벤토리에서 상세 행동 없이 보유 목록만 확인한다
	}

	CloseItemList();
}

void UPRInventoryWidget::HandleInventoryChanged(UPRInventoryComponent* ChangedInventoryComponent,
	const FPRInventoryChangeEventData& EventData)
{
	if (ChangedInventoryComponent != InventoryComponent)
	{
		return;
	}

	RefreshInventoryView();
	RefreshCharacterPreviewWidget();
}

void UPRInventoryWidget::HandleWeaponEquipmentChanged(UPRWeaponManagerComponent* ChangedWeaponManagerComponent, EPRWeaponSlotType ChangedSlot)
{
	static_cast<void>(ChangedSlot);

	if (ChangedWeaponManagerComponent != WeaponManagerComponent)
	{
		return;
	}

	RefreshInventoryView();
	RefreshCharacterPreviewWidget();
}

void UPRInventoryWidget::HandleQuickSlotChanged(UPRQuickSlotComponent* ChangedQuickSlotComponent, int32 ChangedSlotIndex)
{
	static_cast<void>(ChangedSlotIndex);

	if (ChangedQuickSlotComponent != QuickSlotComponent)
	{
		return;
	}

	RefreshQuickSlotWidgets();
}

void UPRInventoryWidget::HandleScrapChanged(int32 NewScrap)
{
	static_cast<void>(NewScrap);

	RefreshCurrencyText();
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

	EPRItemType ItemListType = EPRItemType::Weapon;
	if (PendingWeaponListSlot == EPRWeaponSlotType::Primary)
	{
		ItemListType = EPRItemType::PrimaryWeapon;
	}
	else if (PendingWeaponListSlot == EPRWeaponSlotType::Secondary)
	{
		ItemListType = EPRItemType::SecondaryWeapon;
	}

	// Mod 장착 타겟 무기 Item
	PendingModTargetWeaponItem = nullptr;

	TArray<FPRInventoryItemSlotViewData> ListItems;
	if (IsValid(WeaponManagerComponent->GetWeaponInstanceBySlotType(TargetSlot)))
	{
		ListItems.Add(BuildUnequipViewData(ItemListType));
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

	ItemListWidget->SetItemList(ItemListType, ListItems);
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
		ListItems.Add(BuildUnequipViewData(EPRItemType::Mod));
	}

	for (UPRItemInstance_Mod* ModItem : InventoryComponent->InventoryModItems)
	{
		if (!IsModCompatibleWithWeapon(ModItem, TargetWeaponItem))
		{
			continue;
		}

		const bool bEquipped = ModItem->IsEquipped();
		ListItems.Add(BuildModItemViewData(ModItem, bEquipped));
	}

	ItemListWidget->SetItemList(EPRItemType::Mod, ListItems);
	ItemListWidget->SetVisibility(ESlateVisibility::Visible);
}

void UPRInventoryWidget::OpenConsumableListForQuickSlot(int32 SlotIndex)
{
	if (!IsValid(ItemListWidget) || !IsValid(InventoryComponent) || !IsValid(QuickSlotComponent))
	{
		UE_LOG(LogTemp, Warning, TEXT("[InventoryWidget] 소비 아이템 리스트 열기 실패. OpenConsumableListForQuickSlot()"));
		return;
	}

	PendingWeaponListSlot = EPRWeaponSlotType::None;
	PendingModTargetWeaponItem = nullptr;
	PendingQuickSlotIndex = SlotIndex;

	TArray<FPRInventoryItemSlotViewData> ListItems;
	for (UPRItemInstance_Consumable* ConsumableItem : InventoryComponent->InventoryConsumableItems)
	{
		if (!IsValid(ConsumableItem))
		{
			continue;
		}

		ListItems.Add(BuildConsumableItemViewData(ConsumableItem));
	}

	ItemListWidget->SetItemList(EPRItemType::Consumable, ListItems);
	ItemListWidget->SetVisibility(ESlateVisibility::Visible);
}

void UPRInventoryWidget::OpenMaterialList()
{
	if (!IsValid(ItemListWidget) || !IsValid(InventoryComponent))
	{
		UE_LOG(LogTemp, Warning, TEXT("[InventoryWidget] 재료 아이템 리스트 열기 실패. OpenMaterialList()"));
		return;
	}

	PendingWeaponListSlot = EPRWeaponSlotType::None;
	PendingModTargetWeaponItem = nullptr;
	PendingQuickSlotIndex = INDEX_NONE;

	TArray<FPRInventoryItemSlotViewData> ListItems;
	for (UPRItemInstance_Material* MaterialItem : InventoryComponent->InventoryMaterialItems)
	{
		if (!IsValid(MaterialItem))
		{
			continue;
		}

		ListItems.Add(BuildMaterialItemViewData(MaterialItem));
	}

	ItemListWidget->SetItemList(EPRItemType::Material, ListItems);
	ItemListWidget->SetVisibility(ESlateVisibility::Visible);
}

bool UPRInventoryWidget::IsItemListOpenAs(EPRItemType ListType) const
{
	return IsValid(ItemListWidget) && ItemListWidget->IsVisible() && ItemListWidget->GetListType() == ListType;
}

void UPRInventoryWidget::CloseItemList()
{
	//아이템 리스트 위젯이 유효하면
	if (IsValid(ItemListWidget))
	{
		// 빈 아이템 뷰 데이터 목록 설정
		TArray<FPRInventoryItemSlotViewData> EmptyItems;
		ItemListWidget->SetItemList(EPRItemType::None, EmptyItems);
		ItemListWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	PendingWeaponListSlot = EPRWeaponSlotType::None;
	PendingModTargetWeaponItem = nullptr;
	PendingQuickSlotIndex = INDEX_NONE;
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

void UPRInventoryWidget::RefreshQuickSlotWidgets()
{
	if (IsValid(QuickSlotItemSlotWidget0))
	{
		QuickSlotItemSlotWidget0->SetSlotViewData(BuildQuickSlotViewData(0));
	}

	if (IsValid(QuickSlotItemSlotWidget1))
	{
		QuickSlotItemSlotWidget1->SetSlotViewData(BuildQuickSlotViewData(1));
	}

	if (IsValid(QuickSlotItemSlotWidget2))
	{
		QuickSlotItemSlotWidget2->SetSlotViewData(BuildQuickSlotViewData(2));
	}

	if (IsValid(QuickSlotItemSlotWidget3))
	{
		QuickSlotItemSlotWidget3->SetSlotViewData(BuildQuickSlotViewData(3));
	}
}

void UPRInventoryWidget::RefreshMaterialSlotWidget()
{
	if (IsValid(MaterialSlotWidget))
	{
		MaterialSlotWidget->SetSlotViewData(BuildMaterialSlotViewData());
	}
}

void UPRInventoryWidget::RefreshCurrencyText()
{
	if (!IsValid(ScrapDisplayWidget) && !IsValid(ScrapAmountText))
	{
		return;
	}

	if (!IsValid(CurrencyComponent))
	{
		CurrencyComponent = ResolveCurrencyComponent();
	}

	const int32 ScrapAmount = IsValid(CurrencyComponent) ? CurrencyComponent->GetScrap() : 0;
	if (IsValid(ScrapDisplayWidget))
	{
		ScrapDisplayWidget->SetScrapAmount(ScrapAmount);
	}
	else if (IsValid(ScrapAmountText))
	{
		ScrapAmountText->SetText(FText::AsNumber(ScrapAmount));
	}
}

void UPRInventoryWidget::RefreshInventoryView()
{
	// 장착 슬롯은 리스트 표시 여부와 관계없이 항상 최신 상태로 맞춘다
	RefreshEquippedSlotWidgets();
	RefreshQuickSlotWidgets();
	RefreshMaterialSlotWidget();
	RefreshCurrencyText();

	if (!IsValid(ItemListWidget) || !ItemListWidget->IsVisible())
	{
		return;
	}

	const EPRItemType CurrentListType = ItemListWidget->GetListType();
	if (CurrentListType == EPRItemType::Weapon
		|| CurrentListType == EPRItemType::PrimaryWeapon
		|| CurrentListType == EPRItemType::SecondaryWeapon)
	{
		if (PendingWeaponListSlot != EPRWeaponSlotType::None)
		{
			OpenWeaponList(PendingWeaponListSlot);
		}
	}
	else if (CurrentListType == EPRItemType::Mod && IsValid(PendingModTargetWeaponItem))
	{
		OpenModList(PendingModTargetWeaponItem);
	}
	else if (CurrentListType == EPRItemType::Consumable && PendingQuickSlotIndex != INDEX_NONE)
	{
		OpenConsumableListForQuickSlot(PendingQuickSlotIndex);
	}
	else if (CurrentListType == EPRItemType::Material)
	{
		OpenMaterialList();
	}
}

void UPRInventoryWidget::RefreshCharacterPreviewWidget()
{
	if (!IsValid(CharacterPreviewWidget))
	{
		return;
	}

	APRPlayerCharacter* SourceCharacter = GetPreviewSourceCharacter();
	CharacterPreviewWidget->SetPreviewSources(SourceCharacter, WeaponManagerComponent);
}

APRPlayerCharacter* UPRInventoryWidget::GetPreviewSourceCharacter() const
{
	APlayerController* OwningPlayerController = GetOwningPlayer();
	if (!IsValid(OwningPlayerController))
	{
		return nullptr;
	}

	return Cast<APRPlayerCharacter>(OwningPlayerController->GetPawn());
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

FPRInventoryItemSlotViewData UPRInventoryWidget::BuildConsumableItemViewData(UPRItemInstance_Consumable* ConsumableItem) const
{
	FPRInventoryItemSlotViewData ViewData;
	if (!IsValid(ConsumableItem))
	{
		UE_LOG(LogTemp, Warning, TEXT("[InventoryWidget] 소비 아이템 없음. BuildConsumableItemViewData()"));
		return ViewData;
	}

	UPRConsumableDataAsset* ConsumableData = ConsumableItem->GetConsumableData();
	ViewData.ItemData = ConsumableData;
	ViewData.ItemInstance = ConsumableItem;
	ViewData.ItemType = EPRItemType::Consumable;
	ViewData.StackCount = ConsumableItem->GetStackCount();
	ViewData.bShowStackCount = true;

	if (IsValid(ConsumableData))
	{
		ViewData.DisplayName = ConsumableData->GetDisplayName();
		ViewData.Icon = ConsumableData->GetIcon();
	}

	return ViewData;
}

FPRInventoryItemSlotViewData UPRInventoryWidget::BuildMaterialSlotViewData() const
{
	FPRInventoryItemSlotViewData ViewData;
	ViewData.ItemType = EPRItemType::Material;
	ViewData.DisplayName = FText::FromString(TEXT("재료"));
	return ViewData;
}

FPRInventoryItemSlotViewData UPRInventoryWidget::BuildMaterialItemViewData(UPRItemInstance_Material* MaterialItem) const
{
	FPRInventoryItemSlotViewData ViewData;
	if (!IsValid(MaterialItem))
	{
		UE_LOG(LogTemp, Warning, TEXT("[InventoryWidget] 재료 아이템 없음. BuildMaterialItemViewData()"));
		return ViewData;
	}

	UPRMaterialDataAsset* MaterialData = MaterialItem->GetMaterialData();
	ViewData.ItemData = MaterialData;
	ViewData.ItemInstance = MaterialItem;
	ViewData.ItemType = EPRItemType::Material;
	ViewData.StackCount = MaterialItem->GetStackCount();
	ViewData.bShowStackCount = true;

	if (IsValid(MaterialData))
	{
		ViewData.DisplayName = MaterialData->GetDisplayName();
		ViewData.Icon = MaterialData->GetIcon();
	}

	return ViewData;
}

FPRInventoryItemSlotViewData UPRInventoryWidget::BuildQuickSlotViewData(int32 SlotIndex) const
{
	FPRInventoryItemSlotViewData ViewData;
	ViewData.ItemType = EPRItemType::Consumable;

	if (!IsValid(QuickSlotComponent))
	{
		ViewData.DisplayName = FText::FromString(TEXT("퀵슬롯 비어 있음"));
		return ViewData;
	}

	const FPRQuickSlotViewData QuickSlotViewData = QuickSlotComponent->BuildQuickSlotViewData(SlotIndex);
	ViewData.ItemData = QuickSlotViewData.ConsumableData;
	ViewData.ItemInstance = QuickSlotViewData.ConsumableItem;
	ViewData.Icon = QuickSlotViewData.Icon;
	ViewData.StackCount = QuickSlotViewData.StackCount;
	ViewData.bShowStackCount = QuickSlotViewData.bHasRegisteredItem;

	if (IsValid(QuickSlotViewData.ConsumableData))
	{
		ViewData.DisplayName = QuickSlotViewData.ConsumableData->GetDisplayName();
	}
	else
	{
		ViewData.DisplayName = FText::FromString(TEXT("퀵슬롯 비어 있음"));
	}

	return ViewData;
}

FPRInventoryItemSlotViewData UPRInventoryWidget::BuildUnequipViewData(EPRItemType ListType) const
{
	FPRInventoryItemSlotViewData ViewData;
	ViewData.bUnequipEntry = true;

	if (ListType == EPRItemType::Weapon
		|| ListType == EPRItemType::PrimaryWeapon
		|| ListType == EPRItemType::SecondaryWeapon)
	{
		ViewData.ItemType = EPRItemType::Weapon;
		ViewData.DisplayName = FText::FromString(TEXT("무기 해제"));
	}
	else if (ListType == EPRItemType::Mod)
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

UPRCurrencyComponent* UPRInventoryWidget::ResolveCurrencyComponent() const
{
	if (!IsValid(InventoryComponent))
	{
		return nullptr;
	}

	const APRPlayerState* PlayerState = Cast<APRPlayerState>(InventoryComponent->GetOwner());
	if (!IsValid(PlayerState))
	{
		return nullptr;
	}

	return PlayerState->GetCurrencyComponent();
}

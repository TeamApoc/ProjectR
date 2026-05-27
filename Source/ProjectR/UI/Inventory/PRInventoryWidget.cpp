// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRInventoryWidget.h"

#include "GameFramework/PlayerController.h"
#include "Components/TextBlock.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/ItemSystem/Components/PREquipmentManagerComponent.h"
#include "ProjectR/ItemSystem/Components/PRInventoryComponent.h"
#include "ProjectR/ItemSystem/Data/PRConsumableDataAsset.h"
#include "ProjectR/ItemSystem/Data/PREquipmentDataAsset.h"
#include "ProjectR/ItemSystem/Data/PRItemDataAsset.h"
#include "ProjectR/ItemSystem/Data/PRMaterialDataAsset.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Consumable.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Equipment.h"
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

void UPRInventoryWidget::SetInventorySources(UPRInventoryComponent* InInventoryComponent, UPRWeaponManagerComponent* InWeaponManagerComponent, UPRQuickSlotComponent* InQuickSlotComponent, UPREquipmentManagerComponent* InEquipmentManagerComponent)
{
	UnbindInventorySourceEvents();

	InventoryComponent = InInventoryComponent;
	WeaponManagerComponent = InWeaponManagerComponent;
	QuickSlotComponent = InQuickSlotComponent;
	EquipmentManagerComponent = InEquipmentManagerComponent;

	BindInventorySourceEvents();
	RefreshInventoryView();
	RefreshCharacterPreviewWidget();
}

/*~ UUserWidget Interface ~*/

void UPRInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// UMG BindWidgetOptional 포인터를 반복 처리 가능한 배열로 정리
	CacheChildWidgetLists();
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

void UPRInventoryWidget::CacheChildWidgetLists()
{
	// UMG 슬롯 이름은 Blueprint 배치와 맞추기 위한 고정 지점
	// 실제 갱신과 이벤트 연결은 배열을 통해 같은 코드 경로로 처리
	QuickSlotWidgets.Reset();
	QuickSlotWidgets.Add(QuickSlotItemSlotWidget0);
	QuickSlotWidgets.Add(QuickSlotItemSlotWidget1);
	QuickSlotWidgets.Add(QuickSlotItemSlotWidget2);
	QuickSlotWidgets.Add(QuickSlotItemSlotWidget3);

	// 장비 슬롯 위젯과 슬롯 타입은 같은 인덱스를 공유
	// 클릭 시 ViewData.ContextIndex에 슬롯 타입을 넣어 목록 처리 컨텍스트로 사용
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

	// 퀵슬롯은 슬롯 번호만 다르고 열리는 목록과 선택 처리는 동일한 구조
	for (UPRItemSlotWidget* QuickSlotWidget : QuickSlotWidgets)
	{
		if (IsValid(QuickSlotWidget))
		{
			QuickSlotWidget->OnLeftClicked.AddDynamic(this, &UPRInventoryWidget::HandleQuickSlotLeftClicked);
		}
	}

	// 장비 슬롯은 슬롯 타입만 다르고 목록 구성과 장착 요청 흐름이 동일한 구조
	for (UPRItemSlotWidget* EquipmentSlotWidget : EquipmentSlotWidgets)
	{
		if (IsValid(EquipmentSlotWidget))
		{
			EquipmentSlotWidget->OnLeftClicked.AddDynamic(this, &UPRInventoryWidget::HandleEquipmentSlotLeftClicked);
		}
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

	// 배열 캐시 기준으로 연결한 슬롯 이벤트 정리
	for (UPRItemSlotWidget* QuickSlotWidget : QuickSlotWidgets)
	{
		if (IsValid(QuickSlotWidget))
		{
			QuickSlotWidget->OnLeftClicked.RemoveDynamic(this, &UPRInventoryWidget::HandleQuickSlotLeftClicked);
		}
	}

	for (UPRItemSlotWidget* EquipmentSlotWidget : EquipmentSlotWidgets)
	{
		if (IsValid(EquipmentSlotWidget))
		{
			EquipmentSlotWidget->OnLeftClicked.RemoveDynamic(this, &UPRInventoryWidget::HandleEquipmentSlotLeftClicked);
		}
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

	if (IsValid(EquipmentManagerComponent))
	{
		EquipmentManagerComponent->OnEquipmentChanged.RemoveDynamic(this, &UPRInventoryWidget::HandleEquipmentChanged);
		EquipmentManagerComponent->OnEquipmentChanged.AddDynamic(this, &UPRInventoryWidget::HandleEquipmentChanged);
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

	if (IsValid(EquipmentManagerComponent))
	{
		EquipmentManagerComponent->OnEquipmentChanged.RemoveDynamic(this, &UPRInventoryWidget::HandleEquipmentChanged);
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
	if (IsItemListOpenAs(EPRItemType::Mod) && LastFocusedItem == ViewData.ItemInstance)
	{
		CloseItemList();
		return;
	}
	
	OpenModList( ViewData.ItemInstance);
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

	if (IsItemListOpenAs(EPRItemType::Mod) && LastFocusedItem == WeaponItem)
	{
		CloseItemList();
		return;
	}

	OpenModList(WeaponItem);
}

void UPRInventoryWidget::HandleQuickSlotLeftClicked(const FPRInventoryItemSlotViewData& ViewData)
{
	// QuickSlotWidgets 배열 갱신에서 넣은 슬롯 번호
	const int32 SlotIndex = ViewData.ContextIndex;
	if (SlotIndex == INDEX_NONE)
	{
		return;
	}

	if (IsItemListOpenAs(EPRItemType::Consumable) && LastFocusedIndex == SlotIndex)
	{
		CloseItemList();
		return;
	}

	OpenConsumableListForQuickSlot(SlotIndex);
}

void UPRInventoryWidget::HandleEquipmentSlotLeftClicked(const FPRInventoryItemSlotViewData& ViewData)
{
	// 장비 슬롯은 enum 값을 숫자 컨텍스트로 보관
	const EPREquipmentSlotType SlotType = static_cast<EPREquipmentSlotType>(ViewData.ContextIndex);
	if (SlotType == EPREquipmentSlotType::None)
	{
		return;
	}

	if (IsItemListOpenAs(EPRItemType::Equipment) && LastFocusedIndex == ViewData.ContextIndex)
	{
		CloseItemList();
		return;
	}

	OpenEquipmentListForSlot(SlotType);
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
	
	FPRItemActivationContext ActivationContext;
	ActivationContext.InventoryComponent = InventoryComponent;
	ActivationContext.UserActor = GetOwningPlayer();
	ActivationContext.ContextObject = LastFocusedItem;
	ActivationContext.ContextIndex = LastFocusedIndex;
	
	// 리스트 선택은 여기서 ActivateItem/DeactivateItem 요청으로만 변환함
	// 무기 장착, Mod 장착, 퀵슬롯 등록의 실제 처리는 각 ItemInstance 구현에서 나뉨
	if (ViewData.InventoryAction == EPRInventoryAction::Activate)
	{
		if (IsValid(InventoryComponent) && IsValid(ViewData.ItemInstance.Get()))
		{
			// Widget 외부 공통 활성화 경로
			InventoryComponent->RequestActivateItem(ViewData.ItemInstance.Get(), ActivationContext);
		}
	}
	else if (ViewData.InventoryAction == EPRInventoryAction::Deactivate)
	{
		if (IsValid(InventoryComponent) && IsValid(ViewData.ItemInstance.Get()))
		{
			// Widget 외부 공통 비활성화 경로
			InventoryComponent->RequestDeactivateItem(ViewData.ItemInstance.Get(), ActivationContext);
		}
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
	if (ChangedQuickSlotComponent != QuickSlotComponent)
	{
		return;
	}

	RefreshQuickSlotWidgets();
}

void UPRInventoryWidget::HandleEquipmentChanged(EPREquipmentSlotType ChangedSlot, UPRItemInstance_Equipment* EquipmentItem)
{
	static_cast<void>(ChangedSlot);
	static_cast<void>(EquipmentItem);

	// 장비 변경은 슬롯 표시, 열린 장비 목록의 장착 표시, 캐릭터 프리뷰가 함께 변하는 외부 상태
	RefreshInventoryView();
	RefreshCharacterPreviewWidget();
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
	LastFocusedItem = nullptr;
	LastFocusedIndex = INDEX_NONE;

	TArray<FPRInventoryItemSlotViewData> ListItems;
	UPRItemInstance_Weapon* EquippedWeaponItem = WeaponManagerComponent->GetWeaponInstanceBySlotType(TargetSlot);
	if (IsValid(EquippedWeaponItem))
	{
		// 실제 장착 ItemInstance를 유지하는 비활성화 명령 항목
		ListItems.Add(BuildDeactivateActionViewData(ItemListType, EquippedWeaponItem));
	}

	for (UPRItemInstance_Weapon* WeaponItem : InventoryComponent->GetItemsByType<UPRItemInstance_Weapon>(EPRItemType::Weapon))
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

void UPRInventoryWidget::OpenModList(UPRItemInstance* TargetItem)
{
	UPRItemInstance_Weapon* TargetWeaponItem = Cast<UPRItemInstance_Weapon>(TargetItem);
	
	// 아이템 리스트 위젯, 인벤토리 컴포넌트, 장착 타겟 무기 아이템 유효성 확인
	if (!IsValid(ItemListWidget) || !IsValid(InventoryComponent) || !IsValid(TargetWeaponItem))
	{
		UE_LOG(LogTemp, Warning, TEXT("[InventoryWidget] 모드 리스트 열기 실패. OpenModList()"));
		return;
	}

	PendingWeaponListSlot = EPRWeaponSlotType::None;
	LastFocusedItem = TargetWeaponItem;
	LastFocusedIndex = INDEX_NONE;

	TArray<FPRInventoryItemSlotViewData> ListItems;
	UPRItemInstance_Mod* EquippedModItem = TargetWeaponItem->GetEquippedModItem();
	if (IsValid(EquippedModItem))
	{
		// 실제 장착 Mod ItemInstance를 유지하는 비활성화 명령 항목
		ListItems.Add(BuildDeactivateActionViewData(EPRItemType::Mod, EquippedModItem));
	}

	for (UPRItemInstance_Mod* ModItem : InventoryComponent->GetItemsByType<UPRItemInstance_Mod>(EPRItemType::Mod))
	{
		if (!IsModCompatibleWithWeapon(ModItem, TargetWeaponItem))
		{
			continue;
		}

		const bool bEquipped = ModItem->GetEquippedWeaponItem() == TargetWeaponItem;
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
	LastFocusedItem = nullptr;
	// 소비 아이템 선택 결과를 등록할 퀵슬롯 번호를 숫자 컨텍스트로 보관
	LastFocusedIndex = SlotIndex;

	TArray<FPRInventoryItemSlotViewData> ListItems;
	for (UPRItemInstance_Consumable* ConsumableItem : InventoryComponent->GetItemsByType<UPRItemInstance_Consumable>(EPRItemType::Consumable))
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

void UPRInventoryWidget::OpenEquipmentListForSlot(EPREquipmentSlotType SlotType)
{
	if (!IsValid(ItemListWidget) || !IsValid(InventoryComponent) || !IsValid(EquipmentManagerComponent))
	{
		UE_LOG(LogTemp, Warning, TEXT("[InventoryWidget] 장비 아이템 리스트 열기 실패. OpenEquipmentListForSlot()"));
		return;
	}

	if (SlotType == EPREquipmentSlotType::None)
	{
		return;
	}

	PendingWeaponListSlot = EPRWeaponSlotType::None;
	LastFocusedItem = nullptr;
	// 장비 선택 결과는 ItemInstance의 SlotType으로 처리되지만 목록 재구성에는 열린 슬롯 타입이 필요
	LastFocusedIndex = static_cast<int32>(SlotType);

	TArray<FPRInventoryItemSlotViewData> ListItems;
	UPRItemInstance_Equipment* EquippedItem = EquipmentManagerComponent->GetEquippedItem(SlotType);
	if (IsValid(EquippedItem))
	{
		// 장비 해제 항목도 실제 장착 ItemInstance를 들고 공통 Deactivate 요청으로 이동
		ListItems.Add(BuildDeactivateActionViewData(EPRItemType::Equipment, EquippedItem));
	}

	for (UPRItemInstance_Equipment* EquipmentItem : InventoryComponent->GetItemsByType<UPRItemInstance_Equipment>(EPRItemType::Equipment))
	{
		if (!IsValid(EquipmentItem) || !IsValid(EquipmentItem->GetEquipmentData()))
		{
			continue;
		}

		if (EquipmentItem->GetSlotType() != SlotType)
		{
			continue;
		}

		const bool bEquipped = EquippedItem == EquipmentItem;
		ListItems.Add(BuildEquipmentItemViewData(EquipmentItem, bEquipped));
	}

	ItemListWidget->SetItemList(EPRItemType::Equipment, ListItems);
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
	LastFocusedItem = nullptr;
	LastFocusedIndex = INDEX_NONE;

	TArray<FPRInventoryItemSlotViewData> ListItems;
	for (UPRItemInstance_Material* MaterialItem : InventoryComponent->GetItemsByType<UPRItemInstance_Material>(EPRItemType::Material))
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
	LastFocusedItem = nullptr;
	LastFocusedIndex = INDEX_NONE;
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
	for (int32 SlotIndex = 0; SlotIndex < QuickSlotWidgets.Num(); ++SlotIndex)
	{
		UPRItemSlotWidget* QuickSlotWidget = QuickSlotWidgets[SlotIndex];
		if (IsValid(QuickSlotWidget))
		{
			QuickSlotWidget->SetSlotViewData(BuildQuickSlotViewData(SlotIndex));
		}
	}
}

void UPRInventoryWidget::RefreshEquipmentSlotWidgets()
{
	for (int32 SlotIndex = 0; SlotIndex < EquipmentSlotWidgets.Num(); ++SlotIndex)
	{
		if (!EquipmentSlotTypes.IsValidIndex(SlotIndex))
		{
			continue;
		}

		UPRItemSlotWidget* EquipmentSlotWidget = EquipmentSlotWidgets[SlotIndex];
		if (IsValid(EquipmentSlotWidget))
		{
			EquipmentSlotWidget->SetSlotViewData(BuildEquipmentSlotViewData(EquipmentSlotTypes[SlotIndex]));
		}
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
	RefreshEquipmentSlotWidgets();
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
	else if (CurrentListType == EPRItemType::Mod && IsValid(LastFocusedItem))
	{
		OpenModList(LastFocusedItem);
	}
	else if (CurrentListType == EPRItemType::Consumable && LastFocusedIndex != INDEX_NONE)
	{
		OpenConsumableListForQuickSlot(LastFocusedIndex);
	}
	else if (CurrentListType == EPRItemType::Equipment && LastFocusedIndex != INDEX_NONE)
	{
		OpenEquipmentListForSlot(static_cast<EPREquipmentSlotType>(LastFocusedIndex));
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

	UPRWeaponDataAsset* WeaponData = WeaponItem->GetWeaponData();
	ViewData.ItemData = WeaponData;
	ViewData.ItemInstance = WeaponItem;
	ViewData.ItemType = EPRItemType::Weapon;
	ViewData.InventoryAction = bEquipped ? EPRInventoryAction::Deactivate : EPRInventoryAction::Activate;

	if (IsValid(WeaponData))
	{
		ViewData.DisplayName = WeaponData->GetDisplayName();
		ViewData.Icon = WeaponData->GetIcon();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[InventoryWidget] 무기 아이템 데이터 없음. BuildWeaponItemViewData()"));
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
	ViewData.InventoryAction = bEquipped ? EPRInventoryAction::Deactivate : EPRInventoryAction::Activate;

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
	ViewData.InventoryAction = EPRInventoryAction::Activate;
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
	ViewData.ContextIndex = SlotIndex;

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

FPRInventoryItemSlotViewData UPRInventoryWidget::BuildEquipmentSlotViewData(EPREquipmentSlotType SlotType) const
{
	// 장비 슬롯 클릭은 비어 있어도 해당 슬롯의 장비 목록을 열어야 함
	FPRInventoryItemSlotViewData ViewData;
	ViewData.ItemType = EPRItemType::Equipment;
	ViewData.ContextIndex = static_cast<int32>(SlotType);

	if (IsValid(EquipmentManagerComponent))
	{
		if (UPRItemInstance_Equipment* EquipmentItem = EquipmentManagerComponent->GetEquippedItem(SlotType))
		{
			FPRInventoryItemSlotViewData EquippedViewData = BuildEquipmentItemViewData(EquipmentItem, true);
			EquippedViewData.ContextIndex = static_cast<int32>(SlotType);
			return EquippedViewData;
		}
	}

	ViewData.DisplayName = FText::Format(FText::FromString(TEXT("{0} 없음")), GetEquipmentSlotDisplayName(SlotType));
	return ViewData;
}

FPRInventoryItemSlotViewData UPRInventoryWidget::BuildEquipmentItemViewData(UPRItemInstance_Equipment* EquipmentItem, bool bEquipped) const
{
	FPRInventoryItemSlotViewData ViewData;
	if (!IsValid(EquipmentItem))
	{
		UE_LOG(LogTemp, Warning, TEXT("[InventoryWidget] 장비 아이템 없음. BuildEquipmentItemViewData()"));
		return ViewData;
	}

	UPREquipmentDataAsset* EquipmentData = EquipmentItem->GetEquipmentData();
	const EPREquipmentSlotType SlotType = EquipmentItem->GetSlotType();
	ViewData.ItemData = EquipmentData;
	ViewData.ItemInstance = EquipmentItem;
	ViewData.ItemType = EPRItemType::Equipment;
	ViewData.ContextIndex = static_cast<int32>(SlotType);
	ViewData.InventoryAction = bEquipped ? EPRInventoryAction::Deactivate : EPRInventoryAction::Activate;

	if (IsValid(EquipmentData))
	{
		ViewData.DisplayName = EquipmentData->GetDisplayName();
		ViewData.Icon = EquipmentData->GetIcon();
	}

	return ViewData;
}

FPRInventoryItemSlotViewData UPRInventoryWidget::BuildDeactivateActionViewData(EPRItemType ListType, UPRItemInstance* TargetItem) const
{
	FPRInventoryItemSlotViewData ViewData;
	// 명령 슬롯 선택 시 RequestDeactivateItem으로 전달할 실제 대상
	// 표시 데이터는 명령 항목, 실행 데이터는 대상 ItemInstance로 연결
	ViewData.ItemInstance = TargetItem;
	ViewData.InventoryAction = EPRInventoryAction::Deactivate;

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
	else if (ListType == EPRItemType::Equipment)
	{
		ViewData.ItemType = EPRItemType::Equipment;
		ViewData.DisplayName = FText::FromString(TEXT("장비 해제"));

		if (const UPRItemInstance_Equipment* EquipmentItem = Cast<UPRItemInstance_Equipment>(TargetItem))
		{
			ViewData.ContextIndex = static_cast<int32>(EquipmentItem->GetSlotType());
		}
	}

	return ViewData;
}

FText UPRInventoryWidget::GetEquipmentSlotDisplayName(EPREquipmentSlotType SlotType) const
{
	switch (SlotType)
	{
	case EPREquipmentSlotType::Head:
		return FText::FromString(TEXT("머리"));

	case EPREquipmentSlotType::Body:
		return FText::FromString(TEXT("몸통"));

	case EPREquipmentSlotType::Hands:
		return FText::FromString(TEXT("손"));

	case EPREquipmentSlotType::Legs:
		return FText::FromString(TEXT("다리"));

	case EPREquipmentSlotType::Amulet:
		return FText::FromString(TEXT("목걸이"));

	case EPREquipmentSlotType::Ring1:
		return FText::FromString(TEXT("반지 1"));

	case EPREquipmentSlotType::Ring2:
		return FText::FromString(TEXT("반지 2"));

	case EPREquipmentSlotType::None:
	default:
		return FText::FromString(TEXT("장비"));
	}
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

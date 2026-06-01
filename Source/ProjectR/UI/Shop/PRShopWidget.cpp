// Copyright ProjectR. All Rights Reserved.

#include "PRShopWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "ProjectR/ItemSystem/Data/PRConsumableDataAsset.h"
#include "ProjectR/ItemSystem/Data/PRItemDataAsset.h"
#include "ProjectR/ItemSystem/Data/PRMaterialDataAsset.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Consumable.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Material.h"
#include "ProjectR/Player/PRPlayerController.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "ProjectR/Shop/Components/PRShopComponent.h"
#include "ProjectR/Shop/Data/PRShopDataAsset.h"
#include "ProjectR/System/PRAssetManager.h"
#include "ProjectR/UI/Components/PRUIControllerComponent.h"
#include "ProjectR/UI/Inventory/PRCurrencyDisplayWidget.h"
#include "ProjectR/UI/Shop/PRShopItemDetailWidget.h"
#include "ProjectR/UI/Shop/PRShopItemListWidget.h"
#include "ProjectR/ItemSystem/Data/PRWeaponModDataAsset.h"

UPRShopWidget::UPRShopWidget()
{
	Layer = EPRUILayer::Menu;
	InputMode = EPBUIInputMode::UIOnly;
	bShowMouseCursor = true;
}

void UPRShopWidget::SetShopContext(UPRShopComponent* InShopComponent)
{
	ShopComponent = InShopComponent;
	RefreshPlayerSources();
	OpenBuyTab();
	RefreshHeader();
}

void UPRShopWidget::OpenBuyTab()
{
	CurrentTabType = EPRShopTabType::Buy;
	SelectedItem = FPRShopItemSlotViewData();
	RefreshTabVisuals();
	RefreshItemList();
	RefreshSelectedItemDetails();
	RefreshTransactionButtons();
}

void UPRShopWidget::OpenSellTab()
{
	CurrentTabType = EPRShopTabType::Sell;
	SelectedItem = FPRShopItemSlotViewData();
	RefreshTabVisuals();
	RefreshItemList();
	RefreshSelectedItemDetails();
	RefreshTransactionButtons();
}

void UPRShopWidget::CloseShop()
{
	APRPlayerController* PlayerController = Cast<APRPlayerController>(GetOwningPlayer());
	if (!IsValid(PlayerController))
	{
		return;
	}

	UPRUIControllerComponent* UIController = PlayerController->GetUIController();
	if (!IsValid(UIController))
	{
		return;
	}

	UIController->CloseShop();
}

/*~ UUserWidget Interface ~*/

void UPRShopWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BindChildWidgetEvents();
	RefreshPlayerSources();
	RefreshHeader();
	RefreshTabVisuals();
	RefreshSelectedItemDetails();
	RefreshTransactionButtons();
}

void UPRShopWidget::NativeDestruct()
{
	UnbindSourceEvents();
	UnbindChildWidgetEvents();

	Super::NativeDestruct();
}

void UPRShopWidget::BindChildWidgetEvents()
{
	if (IsValid(BuyTabButton))
	{
		BuyTabButton->OnClicked.RemoveDynamic(this, &UPRShopWidget::HandleBuyTabButtonClicked);
		BuyTabButton->OnClicked.AddDynamic(this, &UPRShopWidget::HandleBuyTabButtonClicked);
	}

	if (IsValid(SellTabButton))
	{
		SellTabButton->OnClicked.RemoveDynamic(this, &UPRShopWidget::HandleSellTabButtonClicked);
		SellTabButton->OnClicked.AddDynamic(this, &UPRShopWidget::HandleSellTabButtonClicked);
	}

	if (IsValid(BuyButton))
	{
		BuyButton->OnClicked.RemoveDynamic(this, &UPRShopWidget::HandleBuyButtonClicked);
		BuyButton->OnClicked.AddDynamic(this, &UPRShopWidget::HandleBuyButtonClicked);
	}

	if (IsValid(SellButton))
	{
		SellButton->OnClicked.RemoveDynamic(this, &UPRShopWidget::HandleSellButtonClicked);
		SellButton->OnClicked.AddDynamic(this, &UPRShopWidget::HandleSellButtonClicked);
	}

	if (IsValid(CloseButton))
	{
		CloseButton->OnClicked.RemoveDynamic(this, &UPRShopWidget::HandleCloseButtonClicked);
		CloseButton->OnClicked.AddDynamic(this, &UPRShopWidget::HandleCloseButtonClicked);
	}

	if (IsValid(ItemListWidget))
	{
		ItemListWidget->OnItemSelected.RemoveDynamic(this, &UPRShopWidget::HandleShopItemSelected);
		ItemListWidget->OnItemSelected.AddDynamic(this, &UPRShopWidget::HandleShopItemSelected);
	}
}

void UPRShopWidget::UnbindChildWidgetEvents()
{
	if (IsValid(BuyTabButton))
	{
		BuyTabButton->OnClicked.RemoveDynamic(this, &UPRShopWidget::HandleBuyTabButtonClicked);
	}

	if (IsValid(SellTabButton))
	{
		SellTabButton->OnClicked.RemoveDynamic(this, &UPRShopWidget::HandleSellTabButtonClicked);
	}

	if (IsValid(BuyButton))
	{
		BuyButton->OnClicked.RemoveDynamic(this, &UPRShopWidget::HandleBuyButtonClicked);
	}

	if (IsValid(SellButton))
	{
		SellButton->OnClicked.RemoveDynamic(this, &UPRShopWidget::HandleSellButtonClicked);
	}

	if (IsValid(CloseButton))
	{
		CloseButton->OnClicked.RemoveDynamic(this, &UPRShopWidget::HandleCloseButtonClicked);
	}

	if (IsValid(ItemListWidget))
	{
		ItemListWidget->OnItemSelected.RemoveDynamic(this, &UPRShopWidget::HandleShopItemSelected);
	}
}

void UPRShopWidget::BindSourceEvents()
{
	if (IsValid(InventoryComponent))
	{
		InventoryComponent->GetOnInventoryChanged().RemoveDynamic(this, &UPRShopWidget::HandleInventoryChanged);
		InventoryComponent->GetOnInventoryChanged().AddDynamic(this, &UPRShopWidget::HandleInventoryChanged);
	}

	if (IsValid(CurrencyComponent))
	{
		CurrencyComponent->OnScrapChanged.RemoveDynamic(this, &UPRShopWidget::HandleScrapChanged);
		CurrencyComponent->OnScrapChanged.AddDynamic(this, &UPRShopWidget::HandleScrapChanged);
	}

	APRPlayerController* PlayerController = Cast<APRPlayerController>(GetOwningPlayer());
	if (IsValid(PlayerController))
	{
		PlayerController->OnShopBuyResult.RemoveDynamic(this, &UPRShopWidget::HandleShopBuyResult);
		PlayerController->OnShopBuyResult.AddDynamic(this, &UPRShopWidget::HandleShopBuyResult);
		PlayerController->OnShopSellResult.RemoveDynamic(this, &UPRShopWidget::HandleShopSellResult);
		PlayerController->OnShopSellResult.AddDynamic(this, &UPRShopWidget::HandleShopSellResult);
	}
}

void UPRShopWidget::UnbindSourceEvents()
{
	if (IsValid(InventoryComponent))
	{
		InventoryComponent->GetOnInventoryChanged().RemoveDynamic(this, &UPRShopWidget::HandleInventoryChanged);
	}

	if (IsValid(CurrencyComponent))
	{
		CurrencyComponent->OnScrapChanged.RemoveDynamic(this, &UPRShopWidget::HandleScrapChanged);
	}

	APRPlayerController* PlayerController = Cast<APRPlayerController>(GetOwningPlayer());
	if (IsValid(PlayerController))
	{
		PlayerController->OnShopBuyResult.RemoveDynamic(this, &UPRShopWidget::HandleShopBuyResult);
		PlayerController->OnShopSellResult.RemoveDynamic(this, &UPRShopWidget::HandleShopSellResult);
	}
}

void UPRShopWidget::RefreshPlayerSources()
{
	UnbindSourceEvents();

	InventoryComponent = nullptr;
	CurrencyComponent = nullptr;

	const APRPlayerController* PlayerController = Cast<APRPlayerController>(GetOwningPlayer());
	APRPlayerState* PlayerState = IsValid(PlayerController) ? PlayerController->GetPlayerState<APRPlayerState>() : nullptr;
	if (IsValid(PlayerState))
	{
		InventoryComponent = PlayerState->GetInventoryComponent();
		CurrencyComponent = PlayerState->GetCurrencyComponent();
	}

	BindSourceEvents();
}

void UPRShopWidget::RefreshItemList()
{
	if (!IsValid(ItemListWidget))
	{
		return;
	}

	const TArray<FPRShopItemSlotViewData> ListItems = CurrentTabType == EPRShopTabType::Buy
		? BuildBuyListItems()
		: BuildSellListItems();
	ItemListWidget->SetShopItemList(CurrentTabType, ListItems);
}

void UPRShopWidget::RefreshHeader()
{
	if (IsValid(ShopNameText))
	{
		UPRShopDataAsset* ShopData = IsValid(ShopComponent) ? ShopComponent->GetShopData() : nullptr;
		ShopNameText->SetText(IsValid(ShopData) ? ShopData->DisplayName : FText::GetEmpty());
	}

	if (IsValid(OwnedScrapText))
	{
		const int32 OwnedScrap = IsValid(CurrencyComponent) ? CurrencyComponent->GetScrap() : 0;
		OwnedScrapText->SetScrapAmount(OwnedScrap);
	}
}

void UPRShopWidget::RefreshSelectedItemDetails()
{
	if (!IsValid(SelectedItemDetailWidget))
	{
		return;
	}

	const int32 OwnedStackCount = GetOwnedStackCount(SelectedItem.ItemViewData.ItemData.Get());
	SelectedItemDetailWidget->SetSelectedItemDetails(SelectedItem, CurrentTabType, OwnedStackCount);
}

void UPRShopWidget::RefreshTransactionButtons()
{
	const bool bCanRequest = CanRequestSelectedTransaction();

	if (IsValid(BuyButton))
	{
		BuyButton->SetVisibility(CurrentTabType == EPRShopTabType::Buy ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		BuyButton->SetIsEnabled(bCanRequest);
	}

	if (IsValid(SellButton))
	{
		SellButton->SetVisibility(CurrentTabType == EPRShopTabType::Sell ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		SellButton->SetIsEnabled(bCanRequest);
	}
}

void UPRShopWidget::RefreshTabVisuals()
{
	if (IsValid(BuyTabSelectedImage))
	{
		BuyTabSelectedImage->SetVisibility(CurrentTabType == EPRShopTabType::Buy ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (IsValid(SellTabSelectedImage))
	{
		SellTabSelectedImage->SetVisibility(CurrentTabType == EPRShopTabType::Sell ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

TArray<FPRShopItemSlotViewData> UPRShopWidget::BuildBuyListItems() const
{
	TArray<FPRShopItemSlotViewData> ListItems;
	UPRShopDataAsset* ShopData = IsValid(ShopComponent) ? ShopComponent->GetShopData() : nullptr;
	if (!IsValid(ShopData))
	{
		return ListItems;
	}

	for (const FPRShopEntry& Entry : ShopData->Entries)
	{
		if (!Entry.bCanShopSellToPlayer)
		{
			continue;
		}

		FPRShopItemSlotViewData ViewData = BuildShopSlotViewData(Entry, EPRShopTabType::Buy);
		if (IsValid(ViewData.ItemViewData.ItemData.Get()))
		{
			ListItems.Add(ViewData);
		}
	}

	return ListItems;
}

TArray<FPRShopItemSlotViewData> UPRShopWidget::BuildSellListItems() const
{
	TArray<FPRShopItemSlotViewData> ListItems;
	UPRShopDataAsset* ShopData = IsValid(ShopComponent) ? ShopComponent->GetShopData() : nullptr;
	if (!IsValid(ShopData) || !IsValid(InventoryComponent))
	{
		return ListItems;
	}

	for (const FPRShopEntry& Entry : ShopData->Entries)
	{
		if (!Entry.bCanPlayerSellToShop)
		{
			continue;
		}

		FPRShopItemSlotViewData ViewData = BuildShopSlotViewData(Entry, EPRShopTabType::Sell);
		UPRItemDataAsset* ItemData = ViewData.ItemViewData.ItemData.Get();
		if (!IsValid(ItemData) || GetOwnedStackCount(ItemData) <= 0)
		{
			continue;
		}

		if (!ItemData->IsA<UPRConsumableDataAsset>() && !ItemData->IsA<UPRMaterialDataAsset>())
		{
			continue;
		}

		ListItems.Add(ViewData);
	}

	return ListItems;
}

FPRShopItemSlotViewData UPRShopWidget::BuildShopSlotViewData(const FPRShopEntry& Entry, EPRShopTabType TabType) const
{
	FPRShopItemSlotViewData ViewData;
	ViewData.EntryId = Entry.EntryId;
	ViewData.TabType = TabType;
	ViewData.bUnlimitedStock = Entry.bUnlimitedStock;
	ViewData.RemainingStock = IsValid(ShopComponent) ? ShopComponent->GetRemainingStock(Entry.EntryId) : 0;

	UPRItemDataAsset* ItemData = Entry.ItemAssetId.IsValid()
		? UPRAssetManager::Get().GetItemDataByPrimaryAssetId(Entry.ItemAssetId)
		: nullptr;
	if (!IsValid(ItemData))
	{
		return ViewData;
	}

	ViewData.ItemViewData.ItemData = ItemData;
	ViewData.ItemViewData.DisplayName = ItemData->GetDisplayName();
	ViewData.ItemViewData.Icon = ItemData->GetIcon();
	ViewData.ItemViewData.ItemType = ItemData->GetItemType();
	ViewData.ItemViewData.StackCount = GetOwnedStackCount(ItemData);
	ViewData.ItemViewData.bShowStackCount = true;
	ViewData.ItemViewData.OwnedStackCount = ViewData.ItemViewData.StackCount;
	ViewData.ItemViewData.bHasOwnedStackCount = true;
	ViewData.bSelected = SelectedItem.EntryId == Entry.EntryId && SelectedItem.TabType == TabType;
	ViewData.UnitScrapPrice = TabType == EPRShopTabType::Buy
		? Entry.BaseScrapPrice
		: IsValid(ShopComponent) ? ShopComponent->CalculateSellScrapPrice(Entry) : 0;
	ViewData.bCanRequestTransaction = ViewData.UnitScrapPrice > 0
		&& (TabType == EPRShopTabType::Sell || Entry.bUnlimitedStock || ViewData.RemainingStock > 0);
	return ViewData;
}

int32 UPRShopWidget::GetOwnedStackCount(const UPRItemDataAsset* ItemData) const
{
	if (!IsValid(InventoryComponent) || !IsValid(ItemData))
	{
		return 0;
	}

	if (const UPRConsumableDataAsset* ConsumableData = Cast<UPRConsumableDataAsset>(ItemData))
	{
		const UPRItemInstance_Consumable* ConsumableItem = InventoryComponent->FindItemByData<UPRItemInstance_Consumable>(ConsumableData);
		return IsValid(ConsumableItem) ? ConsumableItem->GetStackCount() : 0;
	}

	if (const UPRMaterialDataAsset* MaterialData = Cast<UPRMaterialDataAsset>(ItemData))
	{
		const UPRItemInstance_Material* MaterialItem = InventoryComponent->FindItemByData<UPRItemInstance_Material>(MaterialData);
		return IsValid(MaterialItem) ? MaterialItem->GetStackCount() : 0;
	}

	return 0;
}

bool UPRShopWidget::CanRequestSelectedTransaction() const
{
	if (!IsValid(ShopComponent) || SelectedItem.EntryId.IsNone() || !SelectedItem.bCanRequestTransaction)
	{
		return false;
	}

	if (CurrentTabType == EPRShopTabType::Buy)
	{
		const int32 OwnedScrap = IsValid(CurrencyComponent) ? CurrencyComponent->GetScrap() : 0;
		return OwnedScrap >= SelectedItem.UnitScrapPrice;
	}

	return GetOwnedStackCount(SelectedItem.ItemViewData.ItemData.Get()) > 0;
}

void UPRShopWidget::HandleBuyTabButtonClicked()
{
	OpenBuyTab();
}

void UPRShopWidget::HandleSellTabButtonClicked()
{
	OpenSellTab();
}

void UPRShopWidget::HandleBuyButtonClicked()
{
	if (!CanRequestSelectedTransaction())
	{
		return;
	}

	APRPlayerController* PlayerController = Cast<APRPlayerController>(GetOwningPlayer());
	if (!IsValid(PlayerController))
	{
		return;
	}

	PlayerController->RequestBuyShopItem(ShopComponent, SelectedItem.EntryId, 1);
}

void UPRShopWidget::HandleSellButtonClicked()
{
	if (!CanRequestSelectedTransaction())
	{
		return;
	}

	APRPlayerController* PlayerController = Cast<APRPlayerController>(GetOwningPlayer());
	if (!IsValid(PlayerController))
	{
		return;
	}

	PlayerController->RequestSellShopItem(ShopComponent, SelectedItem.EntryId, 1);
}

void UPRShopWidget::HandleCloseButtonClicked()
{
	CloseShop();
}

void UPRShopWidget::HandleShopItemSelected(const FPRShopItemSlotViewData& ViewData)
{
	SelectedItem = ViewData;
	RefreshItemList();
	RefreshSelectedItemDetails();
	RefreshTransactionButtons();
}

void UPRShopWidget::HandleInventoryChanged(UPRInventoryComponent* ChangedInventoryComponent, const FPRInventoryChangeEventData& EventData)
{
	if (ChangedInventoryComponent != InventoryComponent)
	{
		return;
	}

	RefreshItemList();
	RefreshSelectedItemDetails();
	RefreshTransactionButtons();
}

void UPRShopWidget::HandleScrapChanged(int32 NewScrap)
{
	RefreshHeader();
	RefreshTransactionButtons();
}

void UPRShopWidget::HandleShopBuyResult(const FPRShopBuyResult& Result)
{
	if (Result.ShopComponent != ShopComponent)
	{
		return;
	}

	RefreshItemList();
	RefreshHeader();
	RefreshTransactionButtons();
}

void UPRShopWidget::HandleShopSellResult(const FPRShopSellResult& Result)
{
	if (Result.ShopComponent != ShopComponent)
	{
		return;
	}

	RefreshItemList();
	RefreshHeader();
	RefreshTransactionButtons();
}

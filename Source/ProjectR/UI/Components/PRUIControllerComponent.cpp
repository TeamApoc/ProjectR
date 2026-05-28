// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#include "PRUIControllerComponent.h"

#include "Blueprint/UserWidget.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/ItemSystem/Components/PREquipmentManagerComponent.h"
#include "ProjectR/ItemSystem/Components/PRInventoryComponent.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "ProjectR/ItemSystem/Components/PRQuickSlotComponent.h"
#include "ProjectR/UI/HUD/PRHUDWidget.h"
#include "ProjectR/UI/InGameMenu/PRInGameMenuWidget.h"
#include "ProjectR/UI/Inventory/PRInventoryWidget.h"
#include "ProjectR/UI/Growth/PRTraitWindowWidget.h"
#include "ProjectR/UI/PRUIManagerSubsystem.h"
#include "ProjectR/UI/Shop/PRShopWidget.h"
#include "ProjectR/UI/WeaponUpgrade/PRWeaponUpgradeWidget.h"
#include "ProjectR/Shop/Components/PRShopComponent.h"
#include "ProjectR/ItemSystem/Components/PRWeaponUpgradeComponent.h"
#include "ProjectR/ItemSystem/Components/PRWeaponManagerComponent.h"
#include "ProjectR/ItemSystem/Data/PRWeaponDataAsset.h"

UPRUIControllerComponent::UPRUIControllerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPRUIControllerComponent::ToggleInventory()
{
	if (!IsLocalPlayer())
	{
		return;
	}

	UPRUIManagerSubsystem* UIManager = GetUIManager();
	if (!IsValid(UIManager))
	{
		return;
	}

	if (IsValid(InventoryWidget) && InventoryWidget->IsInViewport())
	{
		UIManager->PopUI(InventoryWidget);
		return;
	}

	UPRInventoryWidget* CreatedInventoryWidget = GetOrCreateInventoryWidget();
	if (!IsValid(CreatedInventoryWidget))
	{
		return;
	}

	UPRInventoryComponent* InventoryComponent = GetInventoryComponent();
	UPRWeaponManagerComponent* WeaponManagerComponent = GetWeaponManagerComponent();
	UPRQuickSlotComponent* QuickSlotComponent = GetQuickSlotComponent();
	UPREquipmentManagerComponent* EquipmentManagerComponent = GetEquipmentManagerComponent();
	if (!IsValid(InventoryComponent) || !IsValid(WeaponManagerComponent) || !IsValid(QuickSlotComponent) || !IsValid(EquipmentManagerComponent))
	{
		return;
	}

	CreatedInventoryWidget->SetInventorySources(InventoryComponent, WeaponManagerComponent, QuickSlotComponent, EquipmentManagerComponent);
	UIManager->PushUIInstance(CreatedInventoryWidget);
}

void UPRUIControllerComponent::CloseInventory()
{
	if (!IsLocalPlayer())
	{
		return;
	}

	if (!IsValid(InventoryWidget) || !InventoryWidget->IsInViewport())
	{
		return;
	}

	UPRUIManagerSubsystem* UIManager = GetUIManager();
	if (IsValid(UIManager))
	{
		UIManager->PopUI(InventoryWidget);
	}
	else
	{
		InventoryWidget->RemoveFromParent();
	}
}

void UPRUIControllerComponent::ToggleTraitWindow()
{
	if (!IsLocalPlayer())
	{
		return;
	}

	UPRUIManagerSubsystem* UIManager = GetUIManager();
	if (!IsValid(UIManager))
	{
		return;
	}

	if (IsValid(TraitWindowWidget) && TraitWindowWidget->IsInViewport())
	{
		UIManager->PopUI(TraitWindowWidget);
		return;
	}

	UPRTraitWindowWidget* CreatedTraitWindowWidget = GetOrCreateTraitWindowWidget();
	if (!IsValid(CreatedTraitWindowWidget))
	{
		return;
	}

	APlayerController* PlayerController = GetOwningPlayerController();
	APRPlayerState* PlayerState = IsValid(PlayerController) ? PlayerController->GetPlayerState<APRPlayerState>() : nullptr;
	if (!IsValid(PlayerState))
	{
		return;
	}

	CreatedTraitWindowWidget->SetGrowthSource(PlayerState);
	UIManager->PushUIInstance(CreatedTraitWindowWidget);
}

void UPRUIControllerComponent::CloseTraitWindow()
{
	if (!IsLocalPlayer())
	{
		return;
	}

	if (!IsValid(TraitWindowWidget) || !TraitWindowWidget->IsInViewport())
	{
		return;
	}

	UPRUIManagerSubsystem* UIManager = GetUIManager();
	if (IsValid(UIManager))
	{
		UIManager->PopUI(TraitWindowWidget);
	}
	else
	{
		TraitWindowWidget->RemoveFromParent();
	}
}

void UPRUIControllerComponent::ToggleInGameMenu()
{
	if (!IsLocalPlayer())
	{
		return;
	}

	UPRUIManagerSubsystem* UIManager = GetUIManager();
	if (!IsValid(UIManager))
	{
		return;
	}

	if (IsValid(InGameMenuWidget) && InGameMenuWidget->IsInViewport())
	{
		UIManager->PopUI(InGameMenuWidget);
		return;
	}

	UPRInGameMenuWidget* CreatedInGameMenuWidget = GetOrCreateInGameMenuWidget();
	if (!IsValid(CreatedInGameMenuWidget))
	{
		return;
	}

	UIManager->PushUIInstance(CreatedInGameMenuWidget);
}

void UPRUIControllerComponent::CloseInGameMenu()
{
	if (!IsLocalPlayer())
	{
		return;
	}

	if (!IsValid(InGameMenuWidget) || !InGameMenuWidget->IsInViewport())
	{
		return;
	}

	UPRUIManagerSubsystem* UIManager = GetUIManager();
	if (IsValid(UIManager))
	{
		UIManager->PopUI(InGameMenuWidget);
	}
	else
	{
		InGameMenuWidget->RemoveFromParent();
	}
}

void UPRUIControllerComponent::OpenWeaponUpgrade(UPRWeaponUpgradeComponent* UpgradeComponent)
{
	if (!IsLocalPlayer() || !IsValid(UpgradeComponent))
	{
		return;
	}

	CloseShop();

	UPRUIManagerSubsystem* UIManager = GetUIManager();
	if (!IsValid(UIManager))
	{
		return;
	}

	UPRWeaponUpgradeWidget* CreatedWeaponUpgradeWidget = GetOrCreateWeaponUpgradeWidget();
	if (!IsValid(CreatedWeaponUpgradeWidget))
	{
		return;
	}

	CreatedWeaponUpgradeWidget->SetUpgradeContext(UpgradeComponent);
	UIManager->PushUIInstance(CreatedWeaponUpgradeWidget);
}

void UPRUIControllerComponent::OpenShop(UPRShopComponent* ShopComponent)
{
	if (!IsLocalPlayer() || !IsValid(ShopComponent))
	{
		return;
	}

	CloseWeaponUpgrade();

	UPRUIManagerSubsystem* UIManager = GetUIManager();
	if (!IsValid(UIManager))
	{
		return;
	}

	UPRShopWidget* CreatedShopWidget = GetOrCreateShopWidget();
	if (!IsValid(CreatedShopWidget))
	{
		return;
	}

	CreatedShopWidget->SetShopContext(ShopComponent);
	UIManager->PushUIInstance(CreatedShopWidget);
}

void UPRUIControllerComponent::CloseWeaponUpgrade()
{
	if (!IsLocalPlayer())
	{
		return;
	}

	if (!IsValid(WeaponUpgradeWidget) || !WeaponUpgradeWidget->IsInViewport())
	{
		return;
	}

	UPRUIManagerSubsystem* UIManager = GetUIManager();
	if (IsValid(UIManager))
	{
		UIManager->PopUI(WeaponUpgradeWidget);
	}
	else
	{
		WeaponUpgradeWidget->RemoveFromParent();
	}
}

void UPRUIControllerComponent::CloseShop()
{
	if (!IsLocalPlayer())
	{
		return;
	}

	if (!IsValid(ShopWidget) || !ShopWidget->IsInViewport())
	{
		return;
	}

	UPRUIManagerSubsystem* UIManager = GetUIManager();
	if (IsValid(UIManager))
	{
		UIManager->PopUI(ShopWidget);
	}
	else
	{
		ShopWidget->RemoveFromParent();
	}
}

void UPRUIControllerComponent::ShowWeaponScope()
{
	if (!IsLocalPlayer())
	{
		return;
	}

	bWantsWeaponScopeVisible = true;
	RefreshWeaponScopeWidget();

	if (IsValid(WeaponScopeWidget))
	{
		WeaponScopeWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UPRUIControllerComponent::HideWeaponScope()
{
	bWantsWeaponScopeVisible = false;

	if (IsValid(WeaponScopeWidget))
	{
		WeaponScopeWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UPRUIControllerComponent::ShowLevelUpPopup(int32 PreviousLevel, int32 CurrentLevel)
{
	if (!IsLocalPlayer() || CurrentLevel <= PreviousLevel)
	{
		return;
	}

	if (IsValid(HUDWidget))
	{
		HUDWidget->ShowLevelUpPopup(PreviousLevel, CurrentLevel);
	}
}

void UPRUIControllerComponent::ShowPickupRewardNotification(const FPRPickupNotificationPayload& Payload)
{
	if (!IsLocalPlayer() || Payload.Quantity <= 0)
	{
		return;
	}

	if (IsValid(HUDWidget))
	{
		HUDWidget->ShowPickupRewardNotification(Payload);
	}
}

void UPRUIControllerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CloseInventory();
	InventoryWidget = nullptr;
	CloseTraitWindow();
	TraitWindowWidget = nullptr;
	CloseWeaponUpgrade();
	WeaponUpgradeWidget = nullptr;
	CloseShop();
	ShopWidget = nullptr;
	CloseInGameMenu();
	InGameMenuWidget = nullptr;

	UnbindWeaponManager();
	RemoveWeaponScopeWidget();
	TearDownHUDWidget();

	Super::EndPlay(EndPlayReason);
}

void UPRUIControllerComponent::RefreshForPawn(APawn* InPawn)
{
	if (!IsLocalPlayer())
	{
		return;
	}

	// 새 폰이 없으면 HUD 위젯만 정리하고 종료
	if (!IsValid(InPawn))
	{
		UnbindWeaponManager();
		RemoveWeaponScopeWidget();
		TearDownHUDWidget();
		return;
	}

	// 기존 HUD 위젯 정리 후 새 인스턴스 생성. EventManager 바인딩이 새 위젯 NativeOnInitialized에서 다시 등록됨
	TearDownHUDWidget();
	CreateHUDWidget();

	BindWeaponManager(GetWeaponManagerComponent());
	RefreshWeaponScopeWidget();
}

void UPRUIControllerComponent::RemoveAllWidget()
{
	if (InventoryWidget)
	{
		InventoryWidget->RemoveFromParent();
	}
	if (HUDWidget)
	{
		HUDWidget->RemoveFromParent();
	}
	if (WeaponScopeWidget)
	{
		WeaponScopeWidget->RemoveFromParent();
	}
	if (InGameMenuWidget)
	{
		InGameMenuWidget->RemoveFromParent();
	}
	if (UPRUIManagerSubsystem* UIManager = GetUIManager())
	{
		UIManager->ResetSystem();
	}
}

void UPRUIControllerComponent::HandleWeaponEquipmentChanged(UPRWeaponManagerComponent* WeaponManagerComponent, EPRWeaponSlotType ChangedSlot)
{
	RefreshWeaponScopeWidget();
}

bool UPRUIControllerComponent::IsLocalPlayer() const
{
	const APlayerController* PlayerController = GetOwningPlayerController();
	return IsValid(PlayerController) && PlayerController->IsLocalController();
}

APlayerController* UPRUIControllerComponent::GetOwningPlayerController() const
{
	return Cast<APlayerController>(GetOwner());
}

UPRInventoryComponent* UPRUIControllerComponent::GetInventoryComponent() const
{
	const APlayerController* PlayerController = GetOwningPlayerController();
	if (!IsValid(PlayerController))
	{
		return nullptr;
	}

	APRPlayerState* PRPlayerState = PlayerController->GetPlayerState<APRPlayerState>();
	if (!IsValid(PRPlayerState))
	{
		return nullptr;
	}

	return PRPlayerState->GetInventoryComponent();
}

UPRWeaponManagerComponent* UPRUIControllerComponent::GetWeaponManagerComponent() const
{
	const APlayerController* PlayerController = GetOwningPlayerController();
	if (!IsValid(PlayerController))
	{
		return nullptr;
	}
	
	if (APRPlayerCharacter* Player = Cast<APRPlayerCharacter>(PlayerController->GetPawn()))
	{
		return Player->GetWeaponManager();
	}
	
	return nullptr;
}

UPRQuickSlotComponent* UPRUIControllerComponent::GetQuickSlotComponent() const
{
	const APlayerController* PlayerController = GetOwningPlayerController();
	if (!IsValid(PlayerController))
	{
		return nullptr;
	}

	APRPlayerState* PRPlayerState = PlayerController->GetPlayerState<APRPlayerState>();
	if (!IsValid(PRPlayerState))
	{
		return nullptr;
	}

	return PRPlayerState->GetQuickSlotComponent();
}

UPREquipmentManagerComponent* UPRUIControllerComponent::GetEquipmentManagerComponent() const
{
	const APlayerController* PlayerController = GetOwningPlayerController();
	if (!IsValid(PlayerController))
	{
		return nullptr;
	}

	APRPlayerState* PRPlayerState = PlayerController->GetPlayerState<APRPlayerState>();
	if (!IsValid(PRPlayerState))
	{
		return nullptr;
	}

	return PRPlayerState->GetEquipmentManagerComponent();
}

UPRUIManagerSubsystem* UPRUIControllerComponent::GetUIManager() const
{
	const APlayerController* PlayerController = GetOwningPlayerController();
	if (!IsValid(PlayerController))
	{
		return nullptr;
	}

	ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
	if (!IsValid(LocalPlayer))
	{
		return nullptr;
	}

	return LocalPlayer->GetSubsystem<UPRUIManagerSubsystem>();
}

UPRInventoryWidget* UPRUIControllerComponent::GetOrCreateInventoryWidget()
{
	if (IsValid(InventoryWidget))
	{
		return InventoryWidget;
	}

	APlayerController* PlayerController = GetOwningPlayerController();
	if (!IsValid(PlayerController) || !IsValid(InventoryWidgetClass.Get()))
	{
		return nullptr;
	}

	InventoryWidget = CreateWidget<UPRInventoryWidget>(PlayerController, InventoryWidgetClass);
	return InventoryWidget;
}

UPRWeaponUpgradeWidget* UPRUIControllerComponent::GetOrCreateWeaponUpgradeWidget()
{
	if (IsValid(WeaponUpgradeWidget))
	{
		return WeaponUpgradeWidget;
	}

	APlayerController* PlayerController = GetOwningPlayerController();
	if (!IsValid(PlayerController) || !IsValid(WeaponUpgradeWidgetClass.Get()))
	{
		return nullptr;
	}

	WeaponUpgradeWidget = CreateWidget<UPRWeaponUpgradeWidget>(PlayerController, WeaponUpgradeWidgetClass);
	return WeaponUpgradeWidget;
}

UPRShopWidget* UPRUIControllerComponent::GetOrCreateShopWidget()
{
	if (IsValid(ShopWidget))
	{
		return ShopWidget;
	}

	APlayerController* PlayerController = GetOwningPlayerController();
	if (!IsValid(PlayerController) || !IsValid(ShopWidgetClass.Get()))
	{
		return nullptr;
	}

	ShopWidget = CreateWidget<UPRShopWidget>(PlayerController, ShopWidgetClass);
	return ShopWidget;
}

UPRTraitWindowWidget* UPRUIControllerComponent::GetOrCreateTraitWindowWidget()
{
	if (IsValid(TraitWindowWidget))
	{
		return TraitWindowWidget;
	}

	APlayerController* PlayerController = GetOwningPlayerController();
	if (!IsValid(PlayerController) || !IsValid(TraitWindowWidgetClass.Get()))
	{
		return nullptr;
	}

	TraitWindowWidget = CreateWidget<UPRTraitWindowWidget>(PlayerController, TraitWindowWidgetClass);
	return TraitWindowWidget;
}

UPRInGameMenuWidget* UPRUIControllerComponent::GetOrCreateInGameMenuWidget()
{
	if (IsValid(InGameMenuWidget))
	{
		return InGameMenuWidget;
	}

	APlayerController* PlayerController = GetOwningPlayerController();
	if (!IsValid(PlayerController) || !IsValid(InGameMenuWidgetClass.Get()))
	{
		return nullptr;
	}

	InGameMenuWidget = CreateWidget<UPRInGameMenuWidget>(PlayerController, InGameMenuWidgetClass);
	return InGameMenuWidget;
}

void UPRUIControllerComponent::TearDownHUDWidget()
{
	if (!IsValid(HUDWidget))
	{
		return;
	}

	if (UPRUIManagerSubsystem* UIManager = GetUIManager())
	{
		UIManager->PopUI(HUDWidget);
	}
	else if (HUDWidget->IsInViewport())
	{
		HUDWidget->RemoveFromParent();
	}

	HUDWidget = nullptr;
}

void UPRUIControllerComponent::CreateHUDWidget()
{
	APlayerController* PlayerController = GetOwningPlayerController();
	if (!IsValid(PlayerController) || !IsValid(HUDWidgetClass.Get()))
	{
		return;
	}

	HUDWidget = CreateWidget<UPRHUDWidget>(PlayerController, HUDWidgetClass);
	if (!IsValid(HUDWidget))
	{
		return;
	}

	if (UPRUIManagerSubsystem* UIManager = GetUIManager())
	{
		UIManager->PushUIInstance(HUDWidget);
	}
	else
	{
		// UIManager가 없는 예외 케이스에 대한 폴백
		HUDWidget->AddToViewport();
	}
}

void UPRUIControllerComponent::RefreshWeaponScopeWidget()
{
	UPRWeaponManagerComponent* WeaponManagerComponent = GetWeaponManagerComponent();
	if (!IsValid(WeaponManagerComponent))
	{
		RemoveWeaponScopeWidget();
		return;
	}

	const EPRWeaponSlotType CurrentWeaponSlot = WeaponManagerComponent->GetCurrentWeaponSlot();
	const UPRWeaponDataAsset* WeaponData = WeaponManagerComponent->GetWeaponDataBySlotType(CurrentWeaponSlot);
	TSubclassOf<UUserWidget> ScopeWidgetClass = IsValid(WeaponData) ? WeaponData->ScopeWidgetClass : nullptr;
	if (!IsValid(ScopeWidgetClass.Get()))
	{
		RemoveWeaponScopeWidget();
		return;
	}

	if (IsValid(WeaponScopeWidget) && CurrentScopeWidgetClass == ScopeWidgetClass)
	{
		WeaponScopeWidget->SetVisibility(bWantsWeaponScopeVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		return;
	}

	RemoveWeaponScopeWidget();

	APlayerController* PlayerController = GetOwningPlayerController();
	if (!IsValid(PlayerController))
	{
		return;
	}

	WeaponScopeWidget = CreateWidget<UUserWidget>(PlayerController, ScopeWidgetClass);
	if (!IsValid(WeaponScopeWidget))
	{
		return;
	}

	CurrentScopeWidgetClass = ScopeWidgetClass;
	WeaponScopeWidget->AddToViewport(-1);
	WeaponScopeWidget->SetVisibility(bWantsWeaponScopeVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void UPRUIControllerComponent::RemoveWeaponScopeWidget()
{
	if (IsValid(WeaponScopeWidget))
	{
		WeaponScopeWidget->RemoveFromParent();
	}

	WeaponScopeWidget = nullptr;
	CurrentScopeWidgetClass = nullptr;
}

void UPRUIControllerComponent::BindWeaponManager(UPRWeaponManagerComponent* WeaponManagerComponent)
{
	UnbindWeaponManager();

	if (!IsValid(WeaponManagerComponent))
	{
		return;
	}

	BoundWeaponManager = WeaponManagerComponent;
	BoundWeaponManager->GetOnWeaponEquipmentChanged().AddDynamic(this, &ThisClass::HandleWeaponEquipmentChanged);
}

void UPRUIControllerComponent::UnbindWeaponManager()
{
	if (IsValid(BoundWeaponManager))
	{
		BoundWeaponManager->GetOnWeaponEquipmentChanged().RemoveAll(this);
	}

	BoundWeaponManager = nullptr;
}

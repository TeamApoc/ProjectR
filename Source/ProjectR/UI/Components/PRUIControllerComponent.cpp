// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#include "PRUIControllerComponent.h"

#include "Blueprint/UserWidget.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "ProjectR/Inventory/Components/PRInventoryComponent.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "ProjectR/QuickSlot/Coponents/PRQuickSlotComponent.h"
#include "ProjectR/UI/Inventory/PRInventoryWidget.h"
#include "ProjectR/UI/PRUIManagerSubsystem.h"
#include "ProjectR/Weapon/Components/PRWeaponManagerComponent.h"

UPRUIControllerComponent::UPRUIControllerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPRUIControllerComponent::ToggleInventory()
{
	APlayerController* PlayerController = GetOwningPlayerController();
	if (!IsValid(PlayerController) || !PlayerController->IsLocalController())
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
	if (!IsValid(InventoryComponent) || !IsValid(WeaponManagerComponent) || !IsValid(QuickSlotComponent))
	{
		return;
	}

	CreatedInventoryWidget->SetInventorySources(InventoryComponent, WeaponManagerComponent, QuickSlotComponent);
	UIManager->PushUIInstance(CreatedInventoryWidget);
}

void UPRUIControllerComponent::CloseInventory()
{
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

void UPRUIControllerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CloseInventory();
	InventoryWidget = nullptr;

	Super::EndPlay(EndPlayReason);
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

	APawn* ControlledPawn = PlayerController->GetPawn();
	if (!IsValid(ControlledPawn))
	{
		return nullptr;
	}

	return ControlledPawn->FindComponentByClass<UPRWeaponManagerComponent>();
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

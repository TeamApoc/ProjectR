// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRWeaponHUDWidget.h"

#include "AbilitySystemComponent.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Weapon.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "ProjectR/UI/PRUserInterfaceStatics.h"
#include "ProjectR/UI/WeaponStatusHUD/PRWeaponStatusWidget.h"
#include "ProjectR/Weapon/Components/PRWeaponManagerComponent.h"
#include "ProjectR/Weapon/Data/PRWeaponDataAsset.h"
#include "ProjectR/Weapon/Data/PRWeaponModDataAsset.h"

void UPRWeaponHUDWidget::InitializeWeaponHUD()
{
	APlayerController* OwningPlayerController = GetOwningPlayer();
	if (!IsValid(OwningPlayerController))
	{
		return;
	}

	APRPlayerCharacter* PlayerCharacter = Cast<APRPlayerCharacter>(OwningPlayerController->GetPawn());
	if (!IsValid(PlayerCharacter))
	{
		return;
	}

	APRPlayerState* PlayerState = PlayerCharacter->GetPlayerState<APRPlayerState>();
	if (!IsValid(PlayerState))
	{
		return;
	}

	SetWeaponHUDSources(PlayerCharacter->GetWeaponManager(), PlayerState->GetWeaponSet());
}

void UPRWeaponHUDWidget::SetWeaponHUDSources(UPRWeaponManagerComponent* InWeaponManagerComponent, UPRAttributeSet_Weapon* InWeaponSet)
{
	UnbindWeaponHUDSources();

	WeaponManagerComponent = InWeaponManagerComponent;
	WeaponSet = InWeaponSet;

	if (IsValid(PrimaryWeaponStatusWidget))
	{
		PrimaryWeaponStatusWidget->SetSlotType(EPRWeaponSlotType::Primary);
	}

	if (IsValid(SecondaryWeaponStatusWidget))
	{
		SecondaryWeaponStatusWidget->SetSlotType(EPRWeaponSlotType::Secondary);
	}

	if (UPRWeaponManagerComponent* WeaponManager = WeaponManagerComponent.Get())
	{
		WeaponManager->GetOnWeaponEquipmentChanged().RemoveDynamic(this, &UPRWeaponHUDWidget::HandleWeaponEquipmentChanged);
		WeaponManager->GetOnWeaponEquipmentChanged().AddDynamic(this, &UPRWeaponHUDWidget::HandleWeaponEquipmentChanged);
	}

	if (const UPRAttributeSet_Weapon* CurrentWeaponSet = WeaponSet.Get())
	{
		if (UAbilitySystemComponent* ASC = CurrentWeaponSet->GetOwningAbilitySystemComponent())
		{
			AttributeChangeHandles.Add(ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetPrimaryMagazineAmmoAttribute()).AddUObject(this, &UPRWeaponHUDWidget::HandlePrimaryWeaponAttributeChanged));
			AttributeChangeHandles.Add(ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetPrimaryReserveAmmoAttribute()).AddUObject(this, &UPRWeaponHUDWidget::HandlePrimaryWeaponAttributeChanged));
			AttributeChangeHandles.Add(ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetPrimaryAmmoScaleAttribute()).AddUObject(this, &UPRWeaponHUDWidget::HandlePrimaryWeaponAttributeChanged));
			AttributeChangeHandles.Add(ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetPrimaryModGaugeAttribute()).AddUObject(this, &UPRWeaponHUDWidget::HandlePrimaryWeaponAttributeChanged));
			AttributeChangeHandles.Add(ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetPrimaryMaxModGaugeAttribute()).AddUObject(this, &UPRWeaponHUDWidget::HandlePrimaryWeaponAttributeChanged));
			AttributeChangeHandles.Add(ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetPrimaryModStackAttribute()).AddUObject(this, &UPRWeaponHUDWidget::HandlePrimaryWeaponAttributeChanged));
			AttributeChangeHandles.Add(ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetPrimaryMaxModStackAttribute()).AddUObject(this, &UPRWeaponHUDWidget::HandlePrimaryWeaponAttributeChanged));
			AttributeChangeHandles.Add(ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetSecondaryMagazineAmmoAttribute()).AddUObject(this, &UPRWeaponHUDWidget::HandleSecondaryWeaponAttributeChanged));
			AttributeChangeHandles.Add(ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetSecondaryReserveAmmoAttribute()).AddUObject(this, &UPRWeaponHUDWidget::HandleSecondaryWeaponAttributeChanged));
			AttributeChangeHandles.Add(ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetSecondaryAmmoScaleAttribute()).AddUObject(this, &UPRWeaponHUDWidget::HandleSecondaryWeaponAttributeChanged));
			AttributeChangeHandles.Add(ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetSecondaryModGaugeAttribute()).AddUObject(this, &UPRWeaponHUDWidget::HandleSecondaryWeaponAttributeChanged));
			AttributeChangeHandles.Add(ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetSecondaryMaxModGaugeAttribute()).AddUObject(this, &UPRWeaponHUDWidget::HandleSecondaryWeaponAttributeChanged));
			AttributeChangeHandles.Add(ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetSecondaryModStackAttribute()).AddUObject(this, &UPRWeaponHUDWidget::HandleSecondaryWeaponAttributeChanged));
			AttributeChangeHandles.Add(ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetSecondaryMaxModStackAttribute()).AddUObject(this, &UPRWeaponHUDWidget::HandleSecondaryWeaponAttributeChanged));
		}
	}

	RefreshAllWeaponStatus();
}

void UPRWeaponHUDWidget::RefreshAllWeaponStatus()
{
	RefreshWeaponStatus(EPRWeaponSlotType::Primary);
	RefreshWeaponStatus(EPRWeaponSlotType::Secondary);
}

void UPRWeaponHUDWidget::RefreshWeaponStatus(EPRWeaponSlotType SlotType)
{
	const FPRWeaponStatusViewData ViewData = BuildWeaponStatusViewData(SlotType);

	if (SlotType == EPRWeaponSlotType::Primary)
	{
		if (IsValid(PrimaryWeaponStatusWidget))
		{
			PrimaryWeaponStatusWidget->SetWeaponStatus(ViewData);
		}
		return;
	}

	if (SlotType == EPRWeaponSlotType::Secondary)
	{
		if (IsValid(SecondaryWeaponStatusWidget))
		{
			SecondaryWeaponStatusWidget->SetWeaponStatus(ViewData);
		}
	}
}

FPRWeaponStatusViewData UPRWeaponHUDWidget::BuildWeaponStatusViewData(EPRWeaponSlotType SlotType) const
{
	FPRWeaponStatusViewData ViewData;
	ViewData.SlotType = SlotType;

	const UPRWeaponManagerComponent* WeaponManager = WeaponManagerComponent.Get();
	const UPRAttributeSet_Weapon* CurrentWeaponSet = WeaponSet.Get();
	if (!IsValid(WeaponManager) || !IsValid(CurrentWeaponSet))
	{
		return ViewData;
	}

	const UPRWeaponDataAsset* WeaponData = WeaponManager->GetWeaponDataBySlotType(SlotType);
	const FPRWeaponVisualInfo& VisualInfo = WeaponManager->GetVisualInfoBySlotType(SlotType);
	const UPRWeaponModDataAsset* ModData = VisualInfo.ModData;
	const EPRAmmoType AmmoType = IsValid(WeaponData)
		? WeaponData->GetAmmoType()
		: (SlotType == EPRWeaponSlotType::Primary ? EPRAmmoType::Primary : EPRAmmoType::Secondary);
	const float ModGauge = SlotType == EPRWeaponSlotType::Primary
		? CurrentWeaponSet->GetPrimaryModGauge()
		: CurrentWeaponSet->GetSecondaryModGauge();
	const float MaxModGauge = SlotType == EPRWeaponSlotType::Primary
		? CurrentWeaponSet->GetPrimaryMaxModGauge()
		: CurrentWeaponSet->GetSecondaryMaxModGauge();
	const float ModStack = SlotType == EPRWeaponSlotType::Primary
		? CurrentWeaponSet->GetPrimaryModStack()
		: CurrentWeaponSet->GetSecondaryModStack();

	ViewData.bIsCurrentWeaponSlot = WeaponManager->GetCurrentWeaponSlot() == SlotType;
	ViewData.bHasWeapon = IsValid(WeaponData);
	ViewData.WeaponIcon = IsValid(WeaponData) ? WeaponData->GetIcon() : nullptr;
	ViewData.MagazineAmmo = CurrentWeaponSet->GetDisplayedMagazineAmmo(AmmoType);
	ViewData.ReserveAmmo = CurrentWeaponSet->GetDisplayedReserveAmmo(AmmoType);
	ViewData.bHasMod = IsValid(ModData);
	ViewData.ModIcon = IsValid(ModData) ? ModData->GetIcon() : nullptr;
	ViewData.ModGaugePercent = IsValid(ModData)
		? UPRUserInterfaceStatics::ConvertMinMaxToPercent(ModGauge, MaxModGauge)
		: 0.0f;
	ViewData.ModStackCount = ModStack;

	return ViewData;
}

void UPRWeaponHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UPRWeaponHUDWidget::NativeDestruct()
{
	UnbindWeaponHUDSources();
	Super::NativeDestruct();
}

void UPRWeaponHUDWidget::HandleWeaponEquipmentChanged(UPRWeaponManagerComponent* ChangedWeaponManagerComponent, EPRWeaponSlotType ChangedSlot)
{
	if (ChangedWeaponManagerComponent != WeaponManagerComponent.Get())
	{
		return;
	}

	if (ChangedSlot == EPRWeaponSlotType::None)
	{
		RefreshAllWeaponStatus();
		return;
	}

	RefreshWeaponStatus(ChangedSlot);
}

void UPRWeaponHUDWidget::HandlePrimaryWeaponAttributeChanged(const FOnAttributeChangeData& ChangeData)
{
	RefreshWeaponStatus(EPRWeaponSlotType::Primary);
}

void UPRWeaponHUDWidget::HandleSecondaryWeaponAttributeChanged(const FOnAttributeChangeData& ChangeData)
{
	RefreshWeaponStatus(EPRWeaponSlotType::Secondary);
}

void UPRWeaponHUDWidget::UnbindWeaponHUDSources()
{
	if (UPRWeaponManagerComponent* WeaponManager = WeaponManagerComponent.Get())
	{
		WeaponManager->GetOnWeaponEquipmentChanged().RemoveDynamic(this, &UPRWeaponHUDWidget::HandleWeaponEquipmentChanged);
	}

	if (const UPRAttributeSet_Weapon* CurrentWeaponSet = WeaponSet.Get())
	{
		if (UAbilitySystemComponent* ASC = CurrentWeaponSet->GetOwningAbilitySystemComponent())
		{
			ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetPrimaryMagazineAmmoAttribute()).RemoveAll(this);
			ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetPrimaryReserveAmmoAttribute()).RemoveAll(this);
			ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetPrimaryAmmoScaleAttribute()).RemoveAll(this);
			ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetPrimaryModGaugeAttribute()).RemoveAll(this);
			ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetPrimaryMaxModGaugeAttribute()).RemoveAll(this);
			ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetPrimaryModStackAttribute()).RemoveAll(this);
			ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetPrimaryMaxModStackAttribute()).RemoveAll(this);
			ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetSecondaryMagazineAmmoAttribute()).RemoveAll(this);
			ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetSecondaryReserveAmmoAttribute()).RemoveAll(this);
			ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetSecondaryAmmoScaleAttribute()).RemoveAll(this);
			ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetSecondaryModGaugeAttribute()).RemoveAll(this);
			ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetSecondaryMaxModGaugeAttribute()).RemoveAll(this);
			ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetSecondaryModStackAttribute()).RemoveAll(this);
			ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet_Weapon::GetSecondaryMaxModStackAttribute()).RemoveAll(this);
		}
	}

	AttributeChangeHandles.Reset();
	WeaponManagerComponent.Reset();
	WeaponSet.Reset();
}

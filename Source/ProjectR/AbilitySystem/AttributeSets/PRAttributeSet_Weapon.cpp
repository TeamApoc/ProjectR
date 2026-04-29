// Copyright ProjectR. All Rights Reserved.

#include "PRAttributeSet_Weapon.h"

#include "GameplayEffectExtension.h"
#include "Logging/LogMacros.h"
#include "Net/UnrealNetwork.h"
#include "ProjectR/Weapon/Data/PRWeaponDataAsset.h"
#include "ProjectR/Weapon/Data/PRWeaponModDataAsset.h"

namespace
{
	// 주무기 슬롯 자원 속성 판별
	bool IsPrimarySlotAttribute(const FGameplayAttribute& Attribute)
	{
		return Attribute == UPRAttributeSet_Weapon::GetPrimaryMagazineAmmoAttribute()
			|| Attribute == UPRAttributeSet_Weapon::GetPrimaryReserveAmmoAttribute()
			|| Attribute == UPRAttributeSet_Weapon::GetPrimaryModGaugeAttribute()
			|| Attribute == UPRAttributeSet_Weapon::GetPrimaryModStackAttribute();
	}
	//보조무기 슬롯 자원 속성 판별
	bool IsSecondarySlotAttribute(const FGameplayAttribute& Attribute)
	{
		return Attribute == UPRAttributeSet_Weapon::GetSecondaryMagazineAmmoAttribute()
			|| Attribute == UPRAttributeSet_Weapon::GetSecondaryReserveAmmoAttribute()
			|| Attribute == UPRAttributeSet_Weapon::GetSecondaryModGaugeAttribute()
			|| Attribute == UPRAttributeSet_Weapon::GetSecondaryModStackAttribute();
	}
}

UPRAttributeSet_Weapon::UPRAttributeSet_Weapon()
{
	InitBaseDamage(1.0f);
	InitDamageMultiplier(1.0f);
	InitGroggyDamageMultiplier(1.0f);
}

// =====  UAttributeSet Interface =====
void UPRAttributeSet_Weapon::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (IsPrimarySlotAttribute(Attribute) || IsSecondarySlotAttribute(Attribute)
		|| Attribute == GetBaseDamageAttribute()
		|| Attribute == GetDamageMultiplierAttribute()
		|| Attribute == GetArmorPenetrationAttribute()
		|| Attribute == GetWeakpointMultiplierAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
}

void UPRAttributeSet_Weapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, PrimaryMagazineAmmo, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, PrimaryReserveAmmo, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, PrimaryModGauge, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, PrimaryModStack, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, SecondaryMagazineAmmo, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, SecondaryReserveAmmo, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, SecondaryModGauge, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, SecondaryModStack, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, BaseDamage, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, DamageMultiplier, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, ArmorPenetration, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, WeakpointMultiplier, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, GroggyDamageMultiplier, COND_None, REPNOTIFY_Always);
}

FPRWeaponSlotResourceState UPRAttributeSet_Weapon::BuildSlotResourceState(EPRWeaponSlotType SlotType) const
{
	FPRWeaponSlotResourceState ResourceState;
	ResourceState.SlotType = SlotType;

	if (SlotType == EPRWeaponSlotType::Primary)
	{
		ResourceState.MagazineAmmo = FMath::RoundToInt(GetPrimaryMagazineAmmo());
		ResourceState.ReserveAmmo = FMath::RoundToInt(GetPrimaryReserveAmmo());
		ResourceState.ModGauge = GetPrimaryModGauge();
		ResourceState.ModStack = FMath::RoundToInt(GetPrimaryModStack());
	}
	else if (SlotType == EPRWeaponSlotType::Secondary)
	{
		ResourceState.MagazineAmmo = FMath::RoundToInt(GetSecondaryMagazineAmmo());
		ResourceState.ReserveAmmo = FMath::RoundToInt(GetSecondaryReserveAmmo());
		ResourceState.ModGauge = GetSecondaryModGauge();
		ResourceState.ModStack = FMath::RoundToInt(GetSecondaryModStack());
	}

	return ResourceState;
}

// TODO: 어트리뷰트 수정은 GE를 통해 이루어져야 하지 않을지?
void UPRAttributeSet_Weapon::InitializeSlotResources(EPRWeaponSlotType SlotType, const UPRWeaponDataAsset* WeaponData, const UPRWeaponModDataAsset* ModData)
{
	const int32 MagazineAmmo = IsValid(WeaponData) ? FMath::Max(WeaponData->DefaultMagazineAmmo, 0) : 0;
	const int32 ReserveAmmo = IsValid(WeaponData) ? FMath::Max(WeaponData->DefaultReserveAmmo, 0) : 0;
	const float ModGauge = IsValid(ModData) ? FMath::Max(ModData->MaxGauge, 0.0f) : 0.0f;
	const int32 ModStack = IsValid(ModData) ? FMath::Max(ModData->InitialStack, 0) : 0;

	if (SlotType == EPRWeaponSlotType::Primary)
	{
		SetPrimaryMagazineAmmo(MagazineAmmo);
		SetPrimaryReserveAmmo(ReserveAmmo);
		SetPrimaryModGauge(ModGauge);
		SetPrimaryModStack(ModStack);
	}
	else if (SlotType == EPRWeaponSlotType::Secondary)
	{
		SetSecondaryMagazineAmmo(MagazineAmmo);
		SetSecondaryReserveAmmo(ReserveAmmo);
		SetSecondaryModGauge(ModGauge);
		SetSecondaryModStack(ModStack);
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("WeaponSet InitializeSlotResources Slot=%d Weapon=%s Mod=%s Magazine=%d Reserve=%d ModGauge=%.2f ModStack=%d"),
		static_cast<int32>(SlotType),
		*GetNameSafe(WeaponData),
		*GetNameSafe(ModData),
		MagazineAmmo,
		ReserveAmmo,
		ModGauge,
		ModStack);
}

// TODO: 어트리뷰트 수정은 GE를 통해 이루어져야 하지 않을지?
void UPRAttributeSet_Weapon::ApplySlotResourceDelta(EPRWeaponSlotType SlotType, const FPRWeaponSlotResourceDelta& Delta)
{
	FPRWeaponSlotResourceState ResourceState = BuildSlotResourceState(SlotType);
	const FPRWeaponSlotResourceState PreviousState = ResourceState;
	ResourceState.MagazineAmmo = FMath::Max(ResourceState.MagazineAmmo + Delta.MagazineDelta, 0);
	ResourceState.ReserveAmmo = FMath::Max(ResourceState.ReserveAmmo + Delta.ReserveDelta, 0);
	ResourceState.ModGauge = FMath::Max(ResourceState.ModGauge + Delta.ModGaugeDelta, 0.0f);
	ResourceState.ModStack = FMath::Max(ResourceState.ModStack + Delta.ModStackDelta, 0);

	if (SlotType == EPRWeaponSlotType::Primary)
	{
		SetPrimaryMagazineAmmo(ResourceState.MagazineAmmo);
		SetPrimaryReserveAmmo(ResourceState.ReserveAmmo);
		SetPrimaryModGauge(ResourceState.ModGauge);
		SetPrimaryModStack(ResourceState.ModStack);
	}
	else if (SlotType == EPRWeaponSlotType::Secondary)
	{
		SetSecondaryMagazineAmmo(ResourceState.MagazineAmmo);
		SetSecondaryReserveAmmo(ResourceState.ReserveAmmo);
		SetSecondaryModGauge(ResourceState.ModGauge);
		SetSecondaryModStack(ResourceState.ModStack);
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("WeaponSet ApplySlotResourceDelta Slot=%d Prev(Mag=%d Res=%d Gauge=%.2f Stack=%d) Delta(Mag=%d Res=%d Gauge=%.2f Stack=%d) New(Mag=%d Res=%d Gauge=%.2f Stack=%d)"),
		static_cast<int32>(SlotType),
		PreviousState.MagazineAmmo,
		PreviousState.ReserveAmmo,
		PreviousState.ModGauge,
		PreviousState.ModStack,
		Delta.MagazineDelta,
		Delta.ReserveDelta,
		Delta.ModGaugeDelta,
		Delta.ModStackDelta,
		ResourceState.MagazineAmmo,
		ResourceState.ReserveAmmo,
		ResourceState.ModGauge,
		ResourceState.ModStack);
}

void UPRAttributeSet_Weapon::OnRep_PrimaryMagazineAmmo(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Weapon, PrimaryMagazineAmmo, OldValue);
}

void UPRAttributeSet_Weapon::OnRep_PrimaryReserveAmmo(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Weapon, PrimaryReserveAmmo, OldValue);
}

void UPRAttributeSet_Weapon::OnRep_PrimaryModGauge(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Weapon, PrimaryModGauge, OldValue);
}

void UPRAttributeSet_Weapon::OnRep_PrimaryModStack(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Weapon, PrimaryModStack, OldValue);
}

void UPRAttributeSet_Weapon::OnRep_SecondaryMagazineAmmo(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Weapon, SecondaryMagazineAmmo, OldValue);
}

void UPRAttributeSet_Weapon::OnRep_SecondaryReserveAmmo(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Weapon, SecondaryReserveAmmo, OldValue);
}

void UPRAttributeSet_Weapon::OnRep_SecondaryModGauge(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Weapon, SecondaryModGauge, OldValue);
}

void UPRAttributeSet_Weapon::OnRep_SecondaryModStack(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Weapon, SecondaryModStack, OldValue);
}

void UPRAttributeSet_Weapon::OnRep_BaseDamage(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Weapon, BaseDamage, OldValue);
}

void UPRAttributeSet_Weapon::OnRep_DamageMultiplier(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Weapon, DamageMultiplier, OldValue);
}

void UPRAttributeSet_Weapon::OnRep_ArmorPenetration(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Weapon, ArmorPenetration, OldValue);
}

void UPRAttributeSet_Weapon::OnRep_WeakpointMultiplier(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Weapon, WeakpointMultiplier, OldValue);
}

void UPRAttributeSet_Weapon::OnRep_GroggyDamageMultiplier(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Weapon, GroggyDamageMultiplier, OldValue);
}

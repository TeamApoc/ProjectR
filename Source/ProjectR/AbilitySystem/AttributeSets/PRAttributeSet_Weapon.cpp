// Copyright ProjectR. All Rights Reserved.

#include "PRAttributeSet_Weapon.h"
#include "Logging/LogMacros.h"
#include "Net/UnrealNetwork.h"

namespace
{
	// 주무기 슬롯 자원 속성 판별
	bool IsPrimarySlotAttribute(const FGameplayAttribute& Attribute)
	{
		return Attribute == UPRAttributeSet_Weapon::GetPrimaryMagazineAmmoAttribute()
			|| Attribute == UPRAttributeSet_Weapon::GetPrimaryReserveAmmoAttribute()
			|| Attribute == UPRAttributeSet_Weapon::GetPrimaryAmmoScaleAttribute()
			|| Attribute == UPRAttributeSet_Weapon::GetPrimaryReserveAmmoRatioAttribute()
			|| Attribute == UPRAttributeSet_Weapon::GetPrimaryModGaugeAttribute()
			|| Attribute == UPRAttributeSet_Weapon::GetPrimaryModStackAttribute();
	}

	// 보조무기 슬롯 자원 속성 판별
	bool IsSecondarySlotAttribute(const FGameplayAttribute& Attribute)
	{
		return Attribute == UPRAttributeSet_Weapon::GetSecondaryMagazineAmmoAttribute()
			|| Attribute == UPRAttributeSet_Weapon::GetSecondaryReserveAmmoAttribute()
			|| Attribute == UPRAttributeSet_Weapon::GetSecondaryAmmoScaleAttribute()
			|| Attribute == UPRAttributeSet_Weapon::GetSecondaryReserveAmmoRatioAttribute()
			|| Attribute == UPRAttributeSet_Weapon::GetSecondaryModGaugeAttribute()
			|| Attribute == UPRAttributeSet_Weapon::GetSecondaryModStackAttribute();
	}
}

UPRAttributeSet_Weapon::UPRAttributeSet_Weapon()
{
	InitBaseDamage(1.0f);
	InitDamageMultiplier(1.0f);
	InitGroggyDamageMultiplier(1.0f);
	InitPrimaryAmmoScale(1.0f);
	InitSecondaryAmmoScale(1.0f);
	InitPrimaryReserveAmmoRatio(5.0f);
	InitSecondaryReserveAmmoRatio(5.0f);
	InitWeakpointMultiplier(1.0f);
}

// =====  UPRAttributeSet_Weapon Interface =====
FGameplayAttribute UPRAttributeSet_Weapon::GetMagazineAmmoAttribute(EPRAmmoType AmmoType)
{
	return AmmoType == EPRAmmoType::Primary
		? GetPrimaryMagazineAmmoAttribute()
		: GetSecondaryMagazineAmmoAttribute();
}

FGameplayAttribute UPRAttributeSet_Weapon::GetReserveAmmoAttribute(EPRAmmoType AmmoType)
{
	return AmmoType == EPRAmmoType::Primary
		? GetPrimaryReserveAmmoAttribute()
		: GetSecondaryReserveAmmoAttribute();
}

FGameplayAttribute UPRAttributeSet_Weapon::GetAmmoScaleAttribute(EPRAmmoType AmmoType)
{
	return AmmoType == EPRAmmoType::Primary
		? GetPrimaryAmmoScaleAttribute()
		: GetSecondaryAmmoScaleAttribute();
}

FGameplayAttribute UPRAttributeSet_Weapon::GetReserveAmmoRatioAttribute(EPRAmmoType AmmoType)
{
	return AmmoType == EPRAmmoType::Primary
		? GetPrimaryReserveAmmoRatioAttribute()
		: GetSecondaryReserveAmmoRatioAttribute();
}

float UPRAttributeSet_Weapon::GetMagazineAmmoByType(EPRAmmoType AmmoType) const
{
	return AmmoType == EPRAmmoType::Primary
		? GetPrimaryMagazineAmmo()
		: GetSecondaryMagazineAmmo();
}

float UPRAttributeSet_Weapon::GetReserveAmmoByType(EPRAmmoType AmmoType) const
{
	return AmmoType == EPRAmmoType::Primary
		? GetPrimaryReserveAmmo()
		: GetSecondaryReserveAmmo();
}

float UPRAttributeSet_Weapon::GetAmmoScaleByType(EPRAmmoType AmmoType) const
{
	return AmmoType == EPRAmmoType::Primary
		? GetPrimaryAmmoScale()
		: GetSecondaryAmmoScale();
}

float UPRAttributeSet_Weapon::GetReserveAmmoRatioByType(EPRAmmoType AmmoType) const
{
	return AmmoType == EPRAmmoType::Primary
		? GetPrimaryReserveAmmoRatio()
		: GetSecondaryReserveAmmoRatio();
}

int32 UPRAttributeSet_Weapon::GetDisplayedMagazineAmmo(EPRAmmoType AmmoType) const
{
	const float Scale = GetAmmoScaleByType(AmmoType);
	if (Scale <= KINDA_SMALL_NUMBER)
	{
		return 0;
	}
	return FMath::FloorToInt(GetMagazineAmmoByType(AmmoType) / Scale);
}

int32 UPRAttributeSet_Weapon::GetDisplayedReserveAmmo(EPRAmmoType AmmoType) const
{
	const float Scale = GetAmmoScaleByType(AmmoType);
	if (Scale <= KINDA_SMALL_NUMBER)
	{
		return 0;
	}
	return FMath::FloorToInt(GetReserveAmmoByType(AmmoType) / Scale);
}

float UPRAttributeSet_Weapon::GetMaxReserveAmmoRaw(EPRAmmoType AmmoType) const
{
	return GetReserveAmmoRatioByType(AmmoType) * GetAmmoScaleByType(AmmoType) * MagazineRawBaseline;
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

	// 탄창 raw 값은 baseline 캡으로 클램프
	if (Attribute == GetPrimaryMagazineAmmoAttribute() || Attribute == GetSecondaryMagazineAmmoAttribute())
	{
		NewValue = FMath::Min(NewValue, MagazineRawBaseline);
	}

	// 예비탄 raw 값은 슬롯의 동적 max(Ratio × Scale × Baseline)로 클램프
	if (Attribute == GetPrimaryReserveAmmoAttribute())
	{
		const float MaxReserve = GetMaxReserveAmmoRaw(EPRAmmoType::Primary);
		if (MaxReserve > 0.f)
		{
			NewValue = FMath::Min(NewValue, MaxReserve);
		}
	}
	else if (Attribute == GetSecondaryReserveAmmoAttribute())
	{
		const float MaxReserve = GetMaxReserveAmmoRaw(EPRAmmoType::Secondary);
		if (MaxReserve > 0.f)
		{
			NewValue = FMath::Min(NewValue, MaxReserve);
		}
	}
}

void UPRAttributeSet_Weapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, PrimaryMagazineAmmo, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, PrimaryReserveAmmo, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, PrimaryAmmoScale, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, PrimaryReserveAmmoRatio, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, PrimaryModGauge, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, PrimaryModStack, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, SecondaryMagazineAmmo, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, SecondaryReserveAmmo, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, SecondaryAmmoScale, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, SecondaryReserveAmmoRatio, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, SecondaryModGauge, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, SecondaryModStack, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, BaseDamage, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, DamageMultiplier, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, ArmorPenetration, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, WeakpointMultiplier, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, GroggyDamageMultiplier, COND_None, REPNOTIFY_Always);
}

void UPRAttributeSet_Weapon::OnRep_PrimaryMagazineAmmo(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Weapon, PrimaryMagazineAmmo, OldValue);
}

void UPRAttributeSet_Weapon::OnRep_PrimaryReserveAmmo(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Weapon, PrimaryReserveAmmo, OldValue);
}

void UPRAttributeSet_Weapon::OnRep_PrimaryAmmoScale(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Weapon, PrimaryAmmoScale, OldValue);
}

void UPRAttributeSet_Weapon::OnRep_PrimaryReserveAmmoRatio(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Weapon, PrimaryReserveAmmoRatio, OldValue);
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

void UPRAttributeSet_Weapon::OnRep_SecondaryAmmoScale(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Weapon, SecondaryAmmoScale, OldValue);
}

void UPRAttributeSet_Weapon::OnRep_SecondaryReserveAmmoRatio(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Weapon, SecondaryReserveAmmoRatio, OldValue);
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

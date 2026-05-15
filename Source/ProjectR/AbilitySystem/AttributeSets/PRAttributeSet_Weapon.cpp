// Copyright ProjectR. All Rights Reserved.

#include "PRAttributeSet_Weapon.h"
#include "GameplayEffectExtension.h"
#include "Logging/LogMacros.h"
#include "Net/UnrealNetwork.h"

namespace
{
	// 주무기 슬롯 자원 속성 판별
	bool IsPrimarySlotAttribute(const FGameplayAttribute& Attribute)
	{
		return Attribute == UPRAttributeSet_Weapon::GetPrimaryMagazineAmmoAttribute()
			|| Attribute == UPRAttributeSet_Weapon::GetPrimaryMaxMagazineAmmoAttribute()
			|| Attribute == UPRAttributeSet_Weapon::GetPrimaryReserveAmmoAttribute()
			|| Attribute == UPRAttributeSet_Weapon::GetPrimaryMaxReserveAmmoAttribute()
			|| Attribute == UPRAttributeSet_Weapon::GetPrimaryModGaugeAttribute()
			|| Attribute == UPRAttributeSet_Weapon::GetPrimaryMaxModGaugeAttribute()
			|| Attribute == UPRAttributeSet_Weapon::GetPrimaryModStackAttribute()
			|| Attribute == UPRAttributeSet_Weapon::GetPrimaryMaxModStackAttribute();
	}

	// 보조무기 슬롯 자원 속성 판별
	bool IsSecondarySlotAttribute(const FGameplayAttribute& Attribute)
	{
		return Attribute == UPRAttributeSet_Weapon::GetSecondaryMagazineAmmoAttribute()
			|| Attribute == UPRAttributeSet_Weapon::GetSecondaryMaxMagazineAmmoAttribute()
			|| Attribute == UPRAttributeSet_Weapon::GetSecondaryReserveAmmoAttribute()
			|| Attribute == UPRAttributeSet_Weapon::GetSecondaryMaxReserveAmmoAttribute()
			|| Attribute == UPRAttributeSet_Weapon::GetSecondaryModGaugeAttribute()
			|| Attribute == UPRAttributeSet_Weapon::GetSecondaryMaxModGaugeAttribute()
			|| Attribute == UPRAttributeSet_Weapon::GetSecondaryModStackAttribute()
			|| Attribute == UPRAttributeSet_Weapon::GetSecondaryMaxModStackAttribute();
	}
}

UPRAttributeSet_Weapon::UPRAttributeSet_Weapon()
{
	InitBaseDamage(0.0f);
	InitDamageMultiplier(1.0f);
	InitGroggyDamageMultiplier(0.0f);
	InitWeakpointMultiplier(0.0f);
}

// =====  UPRAttributeSet_Weapon Interface =====
FGameplayAttribute UPRAttributeSet_Weapon::GetMagazineAmmoAttribute(EPRAmmoType AmmoType)
{
	return AmmoType == EPRAmmoType::Primary
		? GetPrimaryMagazineAmmoAttribute()
		: GetSecondaryMagazineAmmoAttribute();
}

FGameplayAttribute UPRAttributeSet_Weapon::GetMaxMagazineAmmoAttribute(EPRAmmoType AmmoType)
{
	return AmmoType == EPRAmmoType::Primary
		? GetPrimaryMaxMagazineAmmoAttribute()
		: GetSecondaryMaxMagazineAmmoAttribute();
}

FGameplayAttribute UPRAttributeSet_Weapon::GetReserveAmmoAttribute(EPRAmmoType AmmoType)
{
	return AmmoType == EPRAmmoType::Primary
		? GetPrimaryReserveAmmoAttribute()
		: GetSecondaryReserveAmmoAttribute();
}

FGameplayAttribute UPRAttributeSet_Weapon::GetMaxReserveAmmoAttribute(EPRAmmoType AmmoType)
{
	return AmmoType == EPRAmmoType::Primary
		? GetPrimaryMaxReserveAmmoAttribute()
		: GetSecondaryMaxReserveAmmoAttribute();
}

float UPRAttributeSet_Weapon::GetMagazineAmmoByType(EPRAmmoType AmmoType) const
{
	return AmmoType == EPRAmmoType::Primary
		? GetPrimaryMagazineAmmo()
		: GetSecondaryMagazineAmmo();
}

float UPRAttributeSet_Weapon::GetMaxMagazineAmmoByType(EPRAmmoType AmmoType) const
{
	return AmmoType == EPRAmmoType::Primary
		? GetPrimaryMaxMagazineAmmo()
		: GetSecondaryMaxMagazineAmmo();
}

float UPRAttributeSet_Weapon::GetReserveAmmoByType(EPRAmmoType AmmoType) const
{
	return AmmoType == EPRAmmoType::Primary
		? GetPrimaryReserveAmmo()
		: GetSecondaryReserveAmmo();
}

float UPRAttributeSet_Weapon::GetMaxReserveAmmoByType(EPRAmmoType AmmoType) const
{
	return AmmoType == EPRAmmoType::Primary
		? GetPrimaryMaxReserveAmmo()
		: GetSecondaryMaxReserveAmmo();
}

// =====  UAttributeSet Interface =====
void UPRAttributeSet_Weapon::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (IsPrimarySlotAttribute(Attribute) || IsSecondarySlotAttribute(Attribute)
		|| Attribute == GetBaseDamageAttribute()
		|| Attribute == GetDamageMultiplierAttribute()
		|| Attribute == GetArmorPenetrationAttribute()
		|| Attribute == GetWeakpointMultiplierAttribute()
		|| Attribute == GetGroggyDamageMultiplierAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}

	if (Attribute == GetPrimaryMagazineAmmoAttribute())
	{
		const float MaxMagazine = GetPrimaryMaxMagazineAmmo();
		NewValue = MaxMagazine > 0.0f ? FMath::Min(NewValue, MaxMagazine) : 0.0f;
	}
	else if (Attribute == GetSecondaryMagazineAmmoAttribute())
	{
		const float MaxMagazine = GetSecondaryMaxMagazineAmmo();
		NewValue = MaxMagazine > 0.0f ? FMath::Min(NewValue, MaxMagazine) : 0.0f;
	}

	if (Attribute == GetPrimaryModGaugeAttribute())
	{
		const float MaxGauge = GetPrimaryMaxModGauge();
		if (MaxGauge > 0.0f)
		{
			NewValue = FMath::Min(NewValue, MaxGauge);
		}
	}
	else if (Attribute == GetSecondaryModGaugeAttribute())
	{
		const float MaxGauge = GetSecondaryMaxModGauge();
		if (MaxGauge > 0.0f)
		{
			NewValue = FMath::Min(NewValue, MaxGauge);
		}
	}
	else if (Attribute == GetPrimaryModStackAttribute())
	{
		const float MaxStack = GetPrimaryMaxModStack();
		if (MaxStack > 0.0f)
		{
			NewValue = FMath::Min(NewValue, MaxStack);
		}
	}
	else if (Attribute == GetSecondaryModStackAttribute())
	{
		const float MaxStack = GetSecondaryMaxModStack();
		if (MaxStack > 0.0f)
		{
			NewValue = FMath::Min(NewValue, MaxStack);
		}
	}

	if (Attribute == GetPrimaryReserveAmmoAttribute())
	{
		const float MaxReserve = GetPrimaryMaxReserveAmmo();
		NewValue = MaxReserve > 0.0f ? FMath::Min(NewValue, MaxReserve) : 0.0f;
	}
	else if (Attribute == GetSecondaryReserveAmmoAttribute())
	{
		const float MaxReserve = GetSecondaryMaxReserveAmmo();
		NewValue = MaxReserve > 0.0f ? FMath::Min(NewValue, MaxReserve) : 0.0f;
	}
}

void UPRAttributeSet_Weapon::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	const FGameplayAttribute& Attribute = Data.EvaluatedData.Attribute;
	if (Attribute == GetPrimaryMagazineAmmoAttribute() || Attribute == GetPrimaryMaxMagazineAmmoAttribute())
	{
		const float MaxMagazine = GetPrimaryMaxMagazineAmmo();
		SetPrimaryMagazineAmmo(MaxMagazine > 0.0f ? FMath::Clamp(GetPrimaryMagazineAmmo(), 0.0f, MaxMagazine) : 0.0f);
	}
	else if (Attribute == GetPrimaryReserveAmmoAttribute() || Attribute == GetPrimaryMaxReserveAmmoAttribute())
	{
		const float MaxReserve = GetPrimaryMaxReserveAmmo();
		SetPrimaryReserveAmmo(MaxReserve > 0.0f ? FMath::Clamp(GetPrimaryReserveAmmo(), 0.0f, MaxReserve) : 0.0f);
	}
	else if (Attribute == GetSecondaryMagazineAmmoAttribute() || Attribute == GetSecondaryMaxMagazineAmmoAttribute())
	{
		const float MaxMagazine = GetSecondaryMaxMagazineAmmo();
		SetSecondaryMagazineAmmo(MaxMagazine > 0.0f ? FMath::Clamp(GetSecondaryMagazineAmmo(), 0.0f, MaxMagazine) : 0.0f);
	}
	else if (Attribute == GetSecondaryReserveAmmoAttribute() || Attribute == GetSecondaryMaxReserveAmmoAttribute())
	{
		const float MaxReserve = GetSecondaryMaxReserveAmmo();
		SetSecondaryReserveAmmo(MaxReserve > 0.0f ? FMath::Clamp(GetSecondaryReserveAmmo(), 0.0f, MaxReserve) : 0.0f);
	}
	else if (Attribute == GetPrimaryModGaugeAttribute())
	{
		const float MaxGauge = GetPrimaryMaxModGauge();
		SetPrimaryModGauge(MaxGauge > 0.0f ? FMath::Clamp(GetPrimaryModGauge(), 0.0f, MaxGauge) : FMath::Max(GetPrimaryModGauge(), 0.0f));
	}
	else if (Attribute == GetPrimaryMaxModGaugeAttribute())
	{
		const float MaxGauge = GetPrimaryMaxModGauge();
		SetPrimaryModGauge(MaxGauge > 0.0f ? FMath::Clamp(GetPrimaryModGauge(), 0.0f, MaxGauge) : 0.0f);
	}
	else if (Attribute == GetSecondaryModGaugeAttribute())
	{
		const float MaxGauge = GetSecondaryMaxModGauge();
		SetSecondaryModGauge(MaxGauge > 0.0f ? FMath::Clamp(GetSecondaryModGauge(), 0.0f, MaxGauge) : FMath::Max(GetSecondaryModGauge(), 0.0f));
	}
	else if (Attribute == GetSecondaryMaxModGaugeAttribute())
	{
		const float MaxGauge = GetSecondaryMaxModGauge();
		SetSecondaryModGauge(MaxGauge > 0.0f ? FMath::Clamp(GetSecondaryModGauge(), 0.0f, MaxGauge) : 0.0f);
	}
	else if (Attribute == GetPrimaryModStackAttribute())
	{
		const float MaxStack = GetPrimaryMaxModStack();
		SetPrimaryModStack(MaxStack > 0.0f ? FMath::Clamp(GetPrimaryModStack(), 0.0f, MaxStack) : FMath::Max(GetPrimaryModStack(), 0.0f));
	}
	else if (Attribute == GetPrimaryMaxModStackAttribute())
	{
		const float MaxStack = GetPrimaryMaxModStack();
		SetPrimaryModStack(MaxStack > 0.0f ? FMath::Clamp(GetPrimaryModStack(), 0.0f, MaxStack) : 0.0f);
	}
	else if (Attribute == GetSecondaryModStackAttribute())
	{
		const float MaxStack = GetSecondaryMaxModStack();
		SetSecondaryModStack(MaxStack > 0.0f ? FMath::Clamp(GetSecondaryModStack(), 0.0f, MaxStack) : FMath::Max(GetSecondaryModStack(), 0.0f));
	}
	else if (Attribute == GetSecondaryMaxModStackAttribute())
	{
		const float MaxStack = GetSecondaryMaxModStack();
		SetSecondaryModStack(MaxStack > 0.0f ? FMath::Clamp(GetSecondaryModStack(), 0.0f, MaxStack) : 0.0f);
	}
}

void UPRAttributeSet_Weapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, PrimaryMagazineAmmo, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, PrimaryMaxMagazineAmmo, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, PrimaryReserveAmmo, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, PrimaryMaxReserveAmmo, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, PrimaryModGauge, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, PrimaryMaxModGauge, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, PrimaryModStack, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, PrimaryMaxModStack, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, SecondaryMagazineAmmo, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, SecondaryMaxMagazineAmmo, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, SecondaryReserveAmmo, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, SecondaryMaxReserveAmmo, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, SecondaryModGauge, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, SecondaryMaxModGauge, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, SecondaryModStack, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, SecondaryMaxModStack, COND_None, REPNOTIFY_Always);
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

void UPRAttributeSet_Weapon::OnRep_PrimaryMaxMagazineAmmo(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Weapon, PrimaryMaxMagazineAmmo, OldValue);
}

void UPRAttributeSet_Weapon::OnRep_PrimaryReserveAmmo(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Weapon, PrimaryReserveAmmo, OldValue);
}

void UPRAttributeSet_Weapon::OnRep_PrimaryMaxReserveAmmo(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Weapon, PrimaryMaxReserveAmmo, OldValue);
}

void UPRAttributeSet_Weapon::OnRep_PrimaryModGauge(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Weapon, PrimaryModGauge, OldValue);
}

void UPRAttributeSet_Weapon::OnRep_PrimaryMaxModGauge(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Weapon, PrimaryMaxModGauge, OldValue);
}

void UPRAttributeSet_Weapon::OnRep_PrimaryModStack(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Weapon, PrimaryModStack, OldValue);
}

void UPRAttributeSet_Weapon::OnRep_PrimaryMaxModStack(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Weapon, PrimaryMaxModStack, OldValue);
}

void UPRAttributeSet_Weapon::OnRep_SecondaryMagazineAmmo(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Weapon, SecondaryMagazineAmmo, OldValue);
}

void UPRAttributeSet_Weapon::OnRep_SecondaryMaxMagazineAmmo(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Weapon, SecondaryMaxMagazineAmmo, OldValue);
}

void UPRAttributeSet_Weapon::OnRep_SecondaryReserveAmmo(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Weapon, SecondaryReserveAmmo, OldValue);
}

void UPRAttributeSet_Weapon::OnRep_SecondaryMaxReserveAmmo(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Weapon, SecondaryMaxReserveAmmo, OldValue);
}

void UPRAttributeSet_Weapon::OnRep_SecondaryModGauge(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Weapon, SecondaryModGauge, OldValue);
}

void UPRAttributeSet_Weapon::OnRep_SecondaryMaxModGauge(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Weapon, SecondaryMaxModGauge, OldValue);
}

void UPRAttributeSet_Weapon::OnRep_SecondaryModStack(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Weapon, SecondaryModStack, OldValue);
}

void UPRAttributeSet_Weapon::OnRep_SecondaryMaxModStack(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Weapon, SecondaryMaxModStack, OldValue);
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

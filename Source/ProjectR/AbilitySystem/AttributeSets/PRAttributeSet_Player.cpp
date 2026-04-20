// Copyright ProjectR. All Rights Reserved.

#include "PRAttributeSet_Player.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"
#include "ProjectR/PRGameplayTags.h"

// =====  UAttributeSet Interface =====

void UPRAttributeSet_Player::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetStaminaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxStamina());
	}
	else if (Attribute == GetModGaugeAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxModGauge());
	}
	else if (Attribute == GetPrimaryAmmoAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxPrimaryAmmo());
	}
	else if (Attribute == GetSecondaryAmmoAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxSecondaryAmmo());
	}
	else if (Attribute == GetStaminaRegenRateAttribute()
		|| Attribute == GetMaxStaminaAttribute()
		|| Attribute == GetMaxModGaugeAttribute()
		|| Attribute == GetMaxPrimaryAmmoAttribute()
		|| Attribute == GetMaxSecondaryAmmoAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
}

void UPRAttributeSet_Player::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	// Stamina 고갈 시 State.StaminaDepleted 태그 부여, 회복 시 제거
	if (Data.EvaluatedData.Attribute == GetStaminaAttribute())
	{
		UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
		if (IsValid(ASC))
		{
			const FGameplayTag DepletedTag = PRGameplayTags::State_StaminaDepleted;
			const bool bHasTag = ASC->HasMatchingGameplayTag(DepletedTag);
			if (GetStamina() <= 0.0f && !bHasTag)
			{
				ASC->AddLooseGameplayTag(DepletedTag);
			}
			else if (GetStamina() > 0.0f && bHasTag)
			{
				ASC->RemoveLooseGameplayTag(DepletedTag);
			}
		}
	}
}

void UPRAttributeSet_Player::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Player, Stamina,          COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Player, MaxStamina,       COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Player, StaminaRegenRate, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Player, ModGauge,         COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Player, MaxModGauge,      COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Player, PrimaryAmmo,      COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Player, MaxPrimaryAmmo,   COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Player, SecondaryAmmo,    COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Player, MaxSecondaryAmmo, COND_None, REPNOTIFY_Always);
}

// =====  OnRep =====

void UPRAttributeSet_Player::OnRep_Stamina(const FGameplayAttributeData& OldValue)          { GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Player, Stamina, OldValue); }
void UPRAttributeSet_Player::OnRep_MaxStamina(const FGameplayAttributeData& OldValue)       { GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Player, MaxStamina, OldValue); }
void UPRAttributeSet_Player::OnRep_StaminaRegenRate(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Player, StaminaRegenRate, OldValue); }
void UPRAttributeSet_Player::OnRep_ModGauge(const FGameplayAttributeData& OldValue)         { GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Player, ModGauge, OldValue); }
void UPRAttributeSet_Player::OnRep_MaxModGauge(const FGameplayAttributeData& OldValue)      { GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Player, MaxModGauge, OldValue); }
void UPRAttributeSet_Player::OnRep_PrimaryAmmo(const FGameplayAttributeData& OldValue)      { GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Player, PrimaryAmmo, OldValue); }
void UPRAttributeSet_Player::OnRep_MaxPrimaryAmmo(const FGameplayAttributeData& OldValue)   { GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Player, MaxPrimaryAmmo, OldValue); }
void UPRAttributeSet_Player::OnRep_SecondaryAmmo(const FGameplayAttributeData& OldValue)    { GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Player, SecondaryAmmo, OldValue); }
void UPRAttributeSet_Player::OnRep_MaxSecondaryAmmo(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Player, MaxSecondaryAmmo, OldValue); }

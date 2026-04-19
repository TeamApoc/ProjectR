// Copyright ProjectR. All Rights Reserved.

#include "PRAttributeSet_Enemy.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"
#include "ProjectR/PRGameplayTags.h"

// =====  UAttributeSet Interface =====

void UPRAttributeSet_Enemy::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetGroggyGaugeAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxGroggyGauge());
	}
	else if (Attribute == GetMaxGroggyGaugeAttribute()
		|| Attribute == GetArmorMultiplierAttribute()
		|| Attribute == GetWeakpointMultiplierAttribute())
	{
		// 음수 금지
		NewValue = FMath::Max(NewValue, 0.0f);
	}
}

void UPRAttributeSet_Enemy::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	// GroggyGauge Max 도달 시 State.Groggy 토글 + OnGroggyStateChanged 발행
	if (Data.EvaluatedData.Attribute == GetGroggyGaugeAttribute())
	{
		UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
		if (IsValid(ASC))
		{
			const FGameplayTag GroggyTag = PRGameplayTags::State_Groggy;
			const bool bHasTag = ASC->HasMatchingGameplayTag(GroggyTag);
			const float Current = GetGroggyGauge();

			if (Current <= 0.0f && !bHasTag)
			{
				ASC->AddLooseGameplayTag(GroggyTag);
				OnGroggyStateChanged.Broadcast(true);
			}
			else if (Current >= GetMaxGroggyGauge() && bHasTag)
			{
				ASC->RemoveLooseGameplayTag(GroggyTag);
				OnGroggyStateChanged.Broadcast(false);
			}
		}
	}
}

void UPRAttributeSet_Enemy::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Enemy, GroggyGauge,         COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Enemy, MaxGroggyGauge,      COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Enemy, ArmorMultiplier,     COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Enemy, WeakpointMultiplier, COND_None, REPNOTIFY_Always);
}

// =====  OnRep =====

void UPRAttributeSet_Enemy::OnRep_GroggyGauge(const FGameplayAttributeData& OldValue)         { GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Enemy, GroggyGauge, OldValue); }
void UPRAttributeSet_Enemy::OnRep_MaxGroggyGauge(const FGameplayAttributeData& OldValue)      { GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Enemy, MaxGroggyGauge, OldValue); }
void UPRAttributeSet_Enemy::OnRep_ArmorMultiplier(const FGameplayAttributeData& OldValue)     { GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Enemy, ArmorMultiplier, OldValue); }
void UPRAttributeSet_Enemy::OnRep_WeakpointMultiplier(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Enemy, WeakpointMultiplier, OldValue); }

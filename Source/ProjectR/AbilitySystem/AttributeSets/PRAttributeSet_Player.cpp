// Copyright ProjectR. All Rights Reserved.

#include "PRAttributeSet_Player.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectExtension.h"
#include "Logging/LogMacros.h"
#include "Net/UnrealNetwork.h"
#include "ProjectR/PRGameplayTags.h"

namespace
{
	float ResolvePoiseDamageMax(float PoiseDamageMin, float PoiseDamageMax)
	{
		return FMath::Max(PoiseDamageMax, PoiseDamageMin);
	}

	float ResolvePoiseThreshold(float Threshold, float MinThreshold, float MaxThreshold)
	{
		return FMath::Clamp(Threshold, MinThreshold, MaxThreshold);
	}

	FGameplayTag ResolveHitReactEventTag(float OldPoiseDamage, float NewPoiseDamage, float IncomingPoiseDamage,
		float WeakHitReactThreshold, float StrongHitReactThreshold, float DownHitReactThreshold)
	{
		if (OldPoiseDamage < DownHitReactThreshold && NewPoiseDamage >= DownHitReactThreshold)
		{
			return PRGameplayTags::Event_Ability_PlayerHitReact_Down;
		}

		if (OldPoiseDamage < StrongHitReactThreshold && NewPoiseDamage >= StrongHitReactThreshold)
		{
			return PRGameplayTags::Event_Ability_PlayerHitReact_Strong;
		}

		if (IncomingPoiseDamage < WeakHitReactThreshold)
		{
			return PRGameplayTags::Event_Ability_PlayerHitReact_Weak;
		}

		return FGameplayTag();
	}

	void SendHitReactEvent(UAbilitySystemComponent* TargetASC, const FGameplayEffectModCallbackData& Data,
		float OldPoiseDamage, float NewPoiseDamage, float IncomingPoiseDamage,
		float WeakHitReactThreshold, float StrongHitReactThreshold, float DownHitReactThreshold)
	{
		if (!IsValid(TargetASC))
		{
			return;
		}

		AActor* AvatarActor = TargetASC->GetAvatarActor();
		if (!IsValid(AvatarActor) || !AvatarActor->HasAuthority())
		{
			return;
		}

		const FGameplayTag EventTag = ResolveHitReactEventTag(
			OldPoiseDamage,
			NewPoiseDamage,
			IncomingPoiseDamage,
			WeakHitReactThreshold,
			StrongHitReactThreshold,
			DownHitReactThreshold);
		if (!EventTag.IsValid())
		{
			return;
		}

		FGameplayEventData Payload;
		Payload.EventTag = EventTag;
		Payload.Instigator = Data.EffectSpec.GetContext().GetOriginalInstigator();
		Payload.Target = AvatarActor;
		Payload.ContextHandle = Data.EffectSpec.GetContext();
		Payload.EventMagnitude = IncomingPoiseDamage;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(AvatarActor, EventTag, Payload);
	}
}

// =====  UAttributeSet Interface =====
void UPRAttributeSet_Player::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetStaminaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxStamina());
	}
	else if (Attribute == GetAccumulatedPoiseDamageAttribute())
	{
		const float ClampMin = FMath::Max(GetPoiseDamageMin(), 0.0f);
		const float ClampMax = ResolvePoiseDamageMax(ClampMin, GetPoiseDamageMax());
		NewValue = FMath::Clamp(NewValue, ClampMin, ClampMax);
	}
	else if (Attribute == GetMaxStaminaAttribute()
		|| Attribute == GetStaminaRegenRateAttribute()
		|| Attribute == GetPoiseDamageMinAttribute()
		|| Attribute == GetPoiseWeakHitReactThresholdAttribute()
		|| Attribute == GetPoiseStrongHitReactThresholdAttribute()
		|| Attribute == GetPoiseDamageMaxAttribute()
		|| Attribute == GetCriticalHitChanceAttribute()
		|| Attribute == GetCriticalDamageMultiplierAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
}

void UPRAttributeSet_Player::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetStaminaAttribute())
	{
		SetStamina(FMath::Clamp(GetStamina(), 0.0f, GetMaxStamina()));
	}
	else if (Data.EvaluatedData.Attribute == GetAccumulatedPoiseDamageAttribute())
	{
		const float ClampMin = FMath::Max(GetPoiseDamageMin(), 0.0f);
		const float ClampMax = ResolvePoiseDamageMax(ClampMin, GetPoiseDamageMax());
		SetAccumulatedPoiseDamage(FMath::Clamp(GetAccumulatedPoiseDamage(), ClampMin, ClampMax));
	}
	else if (Data.EvaluatedData.Attribute == GetIncomingPoiseDamageAttribute())
	{
		const float LocalPoiseDamage = GetIncomingPoiseDamage();
		SetIncomingPoiseDamage(0.0f);
		if (LocalPoiseDamage <= 0.0f)
		{
			return;
		}

		const float ClampMin = FMath::Max(GetPoiseDamageMin(), 0.0f);
		const float ClampMax = ResolvePoiseDamageMax(ClampMin, GetPoiseDamageMax());
		const float WeakHitReactThreshold = ResolvePoiseThreshold(GetPoiseWeakHitReactThreshold(), ClampMin, ClampMax);
		const float StrongHitReactThreshold = ResolvePoiseThreshold(GetPoiseStrongHitReactThreshold(), WeakHitReactThreshold, ClampMax);
		const float OldPoiseDamage = GetAccumulatedPoiseDamage();
		const float NewPoiseDamage = FMath::Clamp(OldPoiseDamage + LocalPoiseDamage, ClampMin, ClampMax);
		SetAccumulatedPoiseDamage(NewPoiseDamage);
		SendHitReactEvent(
			GetOwningAbilitySystemComponent(),
			Data,
			OldPoiseDamage,
			NewPoiseDamage,
			LocalPoiseDamage,
			WeakHitReactThreshold,
			StrongHitReactThreshold,
			ClampMax);
	}
}

void UPRAttributeSet_Player::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Player, Stamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Player, MaxStamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Player, StaminaRegenRate, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Player, AccumulatedPoiseDamage, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Player, PoiseDamageMin, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Player, PoiseWeakHitReactThreshold, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Player, PoiseStrongHitReactThreshold, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Player, PoiseDamageMax, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Player, CriticalHitChance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Player, CriticalDamageMultiplier, COND_None, REPNOTIFY_Always);

}


void UPRAttributeSet_Player::OnRep_Stamina(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Player, Stamina, OldValue);
}

void UPRAttributeSet_Player::OnRep_MaxStamina(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Player, MaxStamina, OldValue);
}

void UPRAttributeSet_Player::OnRep_StaminaRegenRate(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Player, StaminaRegenRate, OldValue);
}

void UPRAttributeSet_Player::OnRep_AccumulatedPoiseDamage(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Player, AccumulatedPoiseDamage, OldValue);
}

void UPRAttributeSet_Player::OnRep_PoiseDamageMin(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Player, PoiseDamageMin, OldValue);
}

void UPRAttributeSet_Player::OnRep_PoiseWeakHitReactThreshold(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Player, PoiseWeakHitReactThreshold, OldValue);
}

void UPRAttributeSet_Player::OnRep_PoiseStrongHitReactThreshold(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Player, PoiseStrongHitReactThreshold, OldValue);
}

void UPRAttributeSet_Player::OnRep_PoiseDamageMax(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Player, PoiseDamageMax, OldValue);
}

void UPRAttributeSet_Player::OnRep_CriticalHitChance(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Player, CriticalHitChance, OldValue);
}

void UPRAttributeSet_Player::OnRep_CriticalDamageMultiplier(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Player, CriticalDamageMultiplier, OldValue);
}

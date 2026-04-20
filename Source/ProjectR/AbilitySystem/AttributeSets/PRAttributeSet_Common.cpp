// Copyright ProjectR. All Rights Reserved.

#include "PRAttributeSet_Common.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"
#include "ProjectR/PRGameplayTags.h"

// =====  UAttributeSet Interface =====

void UPRAttributeSet_Common::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	// Health는 [0, MaxHealth] 범위로 클램프
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	// MovementSpeedMultiplier는 음수 금지
	else if (Attribute == GetMovementSpeedMultiplierAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
}

void UPRAttributeSet_Common::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	// Health => 0 전이 시 State.Dead 부여 + OnDeath 발행
	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		const float NewHealth = GetHealth();
		if (NewHealth <= 0.0f)
		{
			UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
			if (IsValid(ASC))
			{
				const FGameplayTag DeadTag = PRGameplayTags::State_Dead;
				if (!ASC->HasMatchingGameplayTag(DeadTag))
				{
					ASC->AddLooseGameplayTag(DeadTag);

					AActor* Instigator = Data.EffectSpec.GetContext().GetOriginalInstigator();
					OnDeath.Broadcast(Instigator);
				}
			}
		}
	}
}

void UPRAttributeSet_Common::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Common, Health,                   COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Common, MaxHealth,                COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Common, MovementSpeedMultiplier,  COND_None, REPNOTIFY_Always);
}

// =====  OnRep =====

void UPRAttributeSet_Common::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Common, Health, OldValue);
}

void UPRAttributeSet_Common::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Common, MaxHealth, OldValue);
}

void UPRAttributeSet_Common::OnRep_MovementSpeedMultiplier(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Common, MovementSpeedMultiplier, OldValue);
}

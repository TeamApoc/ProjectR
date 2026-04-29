// Copyright ProjectR. All Rights Reserved.

#include "PRAttributeSet_Common.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"
#include "ProjectR/PRGameplayTags.h"


void UPRAttributeSet_Common::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Common, Health,                   COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Common, MaxHealth,                COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Common, MovementSpeedMultiplier,  COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Common, Armor,                    COND_None, REPNOTIFY_Always);
}

// =====  UAttributeSet Interface =====

void UPRAttributeSet_Common::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	// Health는 [0, MaxHealth] 범위로 클램프
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	// MovementSpeedMultiplier 및 Armor는 음수 금지
	else if (Attribute == GetMovementSpeedMultiplierAttribute()
		|| Attribute == GetArmorAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
}

void UPRAttributeSet_Common::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	bool bHealthChanged = false;

	// 메타 어트리뷰트 IncomingDamage 처리
	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		const float LocalDamageDone = GetIncomingDamage();
		SetIncomingDamage(0.0f);

		if (LocalDamageDone > 0.0f)
		{
			const float NewHealth = FMath::Clamp(GetHealth() - LocalDamageDone, 0.0f, GetMaxHealth());
			SetHealth(NewHealth);
			bHealthChanged = true;
		}
	}
	else if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
		bHealthChanged = true;
	}

	// Health => 0 전이 시 State.Dead 부여 + OnDeath 발행
	if (bHealthChanged)
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
					ASC->AddReplicatedLooseGameplayTag(DeadTag);

					AActor* Instigator = Data.EffectSpec.GetContext().GetOriginalInstigator();
					AActor* AvatarActor = ASC->GetAvatarActor();
					if (IsValid(AvatarActor) && AvatarActor->HasAuthority())
					{
						FGameplayEventData Payload;
						Payload.EventTag = PRGameplayTags::Event_Ability_Death;
						Payload.Instigator = Instigator;
						Payload.Target = AvatarActor;

						// 사망 이벤트는 AttributeSet에서 직접 발행해 별도 릴레이 컴포넌트 의존을 줄인다.
						UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
							AvatarActor,
							PRGameplayTags::Event_Ability_Death,
							Payload);
					}

					OnDeath.Broadcast(Instigator);
				}
			}
		}
	}
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

void UPRAttributeSet_Common::OnRep_Armor(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Common, Armor, OldValue);
}

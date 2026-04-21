// Copyright ProjectR. All Rights Reserved.

#include "PRGameplayAbility_EnemyDeath.h"

#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ProjectR/PRGameplayTags.h"

UPRGameplayAbility_EnemyDeath::UPRGameplayAbility_EnemyDeath()
{
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	TriggerData.TriggerTag = PRGameplayTags::Event_Ability_Death;
	AbilityTriggers.Add(TriggerData);

	CancelAbilityTags.AddTag(PRGameplayTags::Ability_Enemy_Pattern);
	CancelAbilityTags.AddTag(PRGameplayTags::Ability_Boss_Faerin_Pattern);
}

void UPRGameplayAbility_EnemyDeath::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->CancelAbilities(&CancelAbilityTags, nullptr, this);
	}

	if (ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		Character->GetCharacterMovement()->DisableMovement();

		if (AAIController* AIController = Cast<AAIController>(Character->GetController()))
		{
			AIController->StopMovement();
		}
	}
}

// Copyright ProjectR. All Rights Reserved.

#include "PRGameplayAbility_EnemyGroggy.h"

#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ProjectR/PRGameplayTags.h"

UPRGameplayAbility_EnemyGroggy::UPRGameplayAbility_EnemyGroggy()
{
	ActivationBlockedTags.AddTag(PRGameplayTags::State_Dead);

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	TriggerData.TriggerTag = PRGameplayTags::Event_Ability_GroggyEntered;
	AbilityTriggers.Add(TriggerData);

	CancelAbilityTags.AddTag(PRGameplayTags::Ability_Enemy_Pattern);
	CancelAbilityTags.AddTag(PRGameplayTags::Ability_Boss_Faerin_Pattern);
}

void UPRGameplayAbility_EnemyGroggy::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
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
		Character->GetCharacterMovement()->StopMovementImmediately();

		if (AAIController* AIController = Cast<AAIController>(Character->GetController()))
		{
			AIController->StopMovement();
		}
	}
}

void UPRGameplayAbility_EnemyGroggy::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->RemoveLooseGameplayTag(PRGameplayTags::State_Groggy);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// Copyright ProjectR. All Rights Reserved.

#include "PRGameplayAbility_BossPhaseTransition.h"

#include "ProjectR/Character/Enemy/PRBossBaseCharacter.h"
#include "ProjectR/PRGameplayTags.h"

UPRGameplayAbility_BossPhaseTransition::UPRGameplayAbility_BossPhaseTransition()
{
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	TriggerData.TriggerTag = PRGameplayTags::Event_Ability_PhaseTransition;
	AbilityTriggers.Add(TriggerData);
}

void UPRGameplayAbility_BossPhaseTransition::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (APRBossBaseCharacter* BossCharacter = Cast<APRBossBaseCharacter>(GetAvatarActorFromActorInfo()))
	{
		BossCharacter->CommitPhaseTransition(TargetPhase);
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

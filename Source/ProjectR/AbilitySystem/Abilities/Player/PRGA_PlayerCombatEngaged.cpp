// Copyright ProjectR. All Rights Reserved.

#include "PRGA_PlayerCombatEngaged.h"

#include "GameplayEffect.h"
#include "ProjectR/PRGameplayTags.h"

UPRGA_PlayerCombatEngaged::UPRGA_PlayerCombatEngaged()
{
	FGameplayTagContainer DefaultAbilityTags;
	DefaultAbilityTags.AddTag(PRGameplayTags::Ability_Player_CombatEngaged);
	SetAssetTags(DefaultAbilityTags);

	FAbilityTriggerData CombatEngagedTriggerData;
	CombatEngagedTriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	CombatEngagedTriggerData.TriggerTag = PRGameplayTags::Event_Combat_Engaged;
	AbilityTriggers.Add(CombatEngagedTriggerData);

	ActivationBlockedTags.AddTag(PRGameplayTags::State_Down);
	ActivationBlockedTags.AddTag(PRGameplayTags::State_Dead);

	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	ActivationPolicy = EPRAbilityActivationPolicy::None;
}

void UPRGA_PlayerCombatEngaged::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ApplyCombatingStateEffect();
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UPRGA_PlayerCombatEngaged::ApplyCombatingStateEffect()
{
	if (!IsValid(CombatingStateEffectClass))
	{
		return;
	}

	const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(CombatingStateEffectClass);
	if (SpecHandle.IsValid())
	{
		ApplyGameplayEffectSpecToOwner(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, SpecHandle);
	}
}

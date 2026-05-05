// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#include "ProjectR/AbilitySystem/Abilities/Player/PRGA_Sprint.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/PRGameplayTags.h"

UPRGA_Sprint::UPRGA_Sprint()
{
	AbilityTags.AddTag(PRGameplayTags::Ability_Player_Sprint);
	ActivationOwnedTags.AddTag(PRGameplayTags::State_Sprinting);
	InputTag = PRGameplayTags::Input_Ability_Sprint;

	ActivationBlockedTags.AddTag(PRGameplayTags::State_Aiming);
	ActivationBlockedTags.AddTag(PRGameplayTags::State_Crouching);
	ActivationBlockedTags.AddTag(PRGameplayTags::State_Dodging);
	ActivationBlockedTags.AddTag(PRGameplayTags::State_StaminaDepleted);

	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Aim);
	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Crouch);
	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Weapon_Fire_Primary);

	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	ActivationPolicy = EPRAbilityActivationPolicy::OnInputTriggered;
}

bool UPRGA_Sprint::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const APRPlayerCharacter* PlayerCharacter = ActorInfo != nullptr
		? Cast<APRPlayerCharacter>(ActorInfo->AvatarActor.Get())
		: nullptr;
	if (!IsValid(PlayerCharacter) || !IsValid(PlayerCharacter->GetCharacterMovement()))
	{
		if (OptionalRelevantTags != nullptr)
		{
			OptionalRelevantTags->AddTag(PRGameplayTags::Fail_Invalid);
		}
		return false;
	}

	if (PlayerCharacter->GetCharacterMovement()->GetCurrentAcceleration().IsNearlyZero())
	{
		if (OptionalRelevantTags != nullptr)
		{
			OptionalRelevantTags->AddTag(PRGameplayTags::Fail_Invalid);
		}
		return false;
	}

	return true;
}

void UPRGA_Sprint::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	StartSprint();
}

void UPRGA_Sprint::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (APRPlayerCharacter* PlayerCharacter = Cast<APRPlayerCharacter>(GetAvatarActorFromActorInfo()))
	{
		PlayerCharacter->SetSprintingFromAbility(false);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UPRGA_Sprint::InputPressed(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	Super::InputPressed(Handle, ActorInfo, ActivationInfo);
}

void UPRGA_Sprint::StartSprint()
{
	APRPlayerCharacter* PlayerCharacter = Cast<APRPlayerCharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(PlayerCharacter))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	PlayerCharacter->SetSprintingFromAbility(true);
}

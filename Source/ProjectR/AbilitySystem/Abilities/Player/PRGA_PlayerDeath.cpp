// Copyright ProjectR. All Rights Reserved.

#include "PRGA_PlayerDeath.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/Game/PRPlayGameMode.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "ProjectR/PRGameplayTags.h"

UPRGA_PlayerDeath::UPRGA_PlayerDeath()
{
	FGameplayTagContainer DefaultAbilityTags;
	DefaultAbilityTags.AddTag(PRGameplayTags::Ability_Player_Death);
	SetAssetTags(DefaultAbilityTags);

	FAbilityTriggerData DeathTriggerData;
	DeathTriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	DeathTriggerData.TriggerTag = PRGameplayTags::Event_Ability_Death;
	AbilityTriggers.Add(DeathTriggerData);

	ActivationBlockedTags.AddTag(PRGameplayTags::State_Dead);

	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Down);
	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Weapon_Fire_Primary);
	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Aim);
	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Crouch);
	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Dodge);
	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Sprint);
	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Reload);
	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Interaction);
	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_HitReact);
	
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Weapon_Fire_Primary);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Aim);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Crouch);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Dodge);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Sprint);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Reload);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Interaction);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_HitReact);

	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
}

void UPRGA_PlayerDeath::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (TriggerEventData != nullptr
		&& TriggerEventData->EventTag.MatchesTagExact(PRGameplayTags::Event_Ability_Death))
	{
		const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
		if (IsValid(ASC) && ASC->HasMatchingGameplayTag(PRGameplayTags::State_Down))
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
			return;
		}
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	EnterDeath();
}

void UPRGA_PlayerDeath::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (IsValid(ActiveMontageTask))
	{
		ActiveMontageTask->EndTask();
		ActiveMontageTask = nullptr;
	}

	ClearDeathStateTags();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UPRGA_PlayerDeath::HandleDeathMontageCompleted()
{
	ActiveMontageTask = nullptr;
}

void UPRGA_PlayerDeath::HandleDeathMontageInterrupted()
{
	ActiveMontageTask = nullptr;
}

void UPRGA_PlayerDeath::EnterDeath()
{
	ApplyDeathStateTags();

	if (ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		UCharacterMovementComponent* CharacterMovement = Character->GetCharacterMovement();
		if (IsValid(CharacterMovement))
		{
			CharacterMovement->StopMovementImmediately();
			CharacterMovement->DisableMovement();
		}
	}

	NotifyDeathToGameMode();

	if (IsValid(ActiveMontageTask))
	{
		ActiveMontageTask->EndTask();
		ActiveMontageTask = nullptr;
	}

	if (!IsValid(DeathMontage))
	{
		return;
	}

	ActiveMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		DeathMontage,
		FMath::Max(MontagePlayRate, UE_SMALL_NUMBER),
		NAME_None,
		true);
	if (!IsValid(ActiveMontageTask))
	{
		return;
	}

	ActiveMontageTask->OnCompleted.AddDynamic(this, &UPRGA_PlayerDeath::HandleDeathMontageCompleted);
	ActiveMontageTask->OnBlendOut.AddDynamic(this, &UPRGA_PlayerDeath::HandleDeathMontageCompleted);
	ActiveMontageTask->OnInterrupted.AddDynamic(this, &UPRGA_PlayerDeath::HandleDeathMontageInterrupted);
	ActiveMontageTask->OnCancelled.AddDynamic(this, &UPRGA_PlayerDeath::HandleDeathMontageInterrupted);
	ActiveMontageTask->ReadyForActivation();
}

void UPRGA_PlayerDeath::ApplyDeathStateTags()
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor) || !AvatarActor->HasAuthority() || bDeathStateTagsAdded)
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(ASC))
	{
		return;
	}

	ASC->AddLooseGameplayTag(PRGameplayTags::State_Dead);
	ASC->AddLooseGameplayTag(PRGameplayTags::State_Block_Move);
	ASC->AddLooseGameplayTag(PRGameplayTags::State_PlayerInputLocked);
	ASC->AddReplicatedLooseGameplayTag(PRGameplayTags::State_Dead);
	ASC->AddReplicatedLooseGameplayTag(PRGameplayTags::State_Block_Move);
	ASC->AddReplicatedLooseGameplayTag(PRGameplayTags::State_PlayerInputLocked);
	bDeathStateTagsAdded = true;
}

void UPRGA_PlayerDeath::ClearDeathStateTags()
{
	if (!bDeathStateTagsAdded)
	{
		return;
	}

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (IsValid(AvatarActor) && AvatarActor->HasAuthority() && IsValid(ASC))
	{
		ASC->RemoveLooseGameplayTag(PRGameplayTags::State_Dead);
		ASC->RemoveLooseGameplayTag(PRGameplayTags::State_Block_Move);
		ASC->RemoveLooseGameplayTag(PRGameplayTags::State_PlayerInputLocked);
		ASC->RemoveReplicatedLooseGameplayTag(PRGameplayTags::State_Dead);
		ASC->RemoveReplicatedLooseGameplayTag(PRGameplayTags::State_Block_Move);
		ASC->RemoveReplicatedLooseGameplayTag(PRGameplayTags::State_PlayerInputLocked);
	}

	bDeathStateTagsAdded = false;
}

void UPRGA_PlayerDeath::NotifyDeathToGameMode() const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor) || !AvatarActor->HasAuthority())
	{
		return;
	}

	UWorld* World = GetWorld();
	APRPlayGameMode* PlayGameMode = IsValid(World) ? World->GetAuthGameMode<APRPlayGameMode>() : nullptr;
	if (!IsValid(PlayGameMode))
	{
		return;
	}

	const APRPlayerCharacter* PlayerCharacter = Cast<APRPlayerCharacter>(AvatarActor);
	APRPlayerState* PlayerState = IsValid(PlayerCharacter) ? PlayerCharacter->GetPlayerState<APRPlayerState>() : nullptr;
	PlayGameMode->NotifyPlayerSurvivalStateChanged(PlayerState);
}

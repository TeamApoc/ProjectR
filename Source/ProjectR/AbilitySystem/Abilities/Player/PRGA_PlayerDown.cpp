// Copyright ProjectR. All Rights Reserved.

#include "PRGA_PlayerDown.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/Game/PRPlayGameMode.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "ProjectR/PRGameplayTags.h"
#include "TimerManager.h"

UPRGA_PlayerDown::UPRGA_PlayerDown()
{
	FGameplayTagContainer DefaultAbilityTags;
	DefaultAbilityTags.AddTag(PRGameplayTags::Ability_Player_Down);
	SetAssetTags(DefaultAbilityTags);

	FAbilityTriggerData DownTriggerData;
	DownTriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	DownTriggerData.TriggerTag = PRGameplayTags::Event_Ability_Down;
	AbilityTriggers.Add(DownTriggerData);

	ActivationOwnedTags.AddTag(PRGameplayTags::State_Down);
	ActivationOwnedTags.AddTag(PRGameplayTags::State_PlayerInputLocked);
	ActivationBlockedTags.AddTag(PRGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(PRGameplayTags::State_Down);

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

void UPRGA_PlayerDown::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	bDownToDeathStarted = false;
	bDownEnterMovementBlockAdded = false;
	bDownDeathFinalized = false;

	if (APRPlayerCharacter* PlayerCharacter = Cast<APRPlayerCharacter>(GetAvatarActorFromActorInfo()))
	{
		UCharacterMovementComponent* CharacterMovement = PlayerCharacter->GetCharacterMovement();
		if (IsValid(CharacterMovement))
		{
			CharacterMovement->StopMovementImmediately();
		}
	}

	PlayDownEnterMontage();
	StartDownTimer();
	StartDownDeathEventWait();
	NotifyDownToGameMode();
}

void UPRGA_PlayerDown::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	StopDownTimer();
	StopDownDeathEventWait();
	StopDownToDeathFinalizeTimer();
	SetDownEnterMovementBlocked(false);

	if (IsValid(ActiveMontageTask))
	{
		ActiveMontageTask->EndTask();
		ActiveMontageTask = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	bDownToDeathStarted = false;
}

void UPRGA_PlayerDown::HandleDownMontageCompleted()
{
	ActiveMontageTask = nullptr;
	SetDownEnterMovementBlocked(false);
}

void UPRGA_PlayerDown::HandleDownMontageInterrupted()
{
	ActiveMontageTask = nullptr;
	SetDownEnterMovementBlocked(false);
}

void UPRGA_PlayerDown::HandleDownDeathEvent(FGameplayEventData EventData)
{
	StartDownToDeath();
}

void UPRGA_PlayerDown::HandleDownToDeathMontageCompleted()
{
	ActiveMontageTask = nullptr;
	StopDownToDeathFinalizeTimer();
	FinalizeDownDeath();
}

void UPRGA_PlayerDown::HandleDownToDeathMontageInterrupted()
{
	ActiveMontageTask = nullptr;
	StopDownToDeathFinalizeTimer();
	FinalizeDownDeath();
}

void UPRGA_PlayerDown::StartDownTimer()
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor) || !AvatarActor->HasAuthority())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	World->GetTimerManager().SetTimer(
		DownTimerHandle,
		this,
		&UPRGA_PlayerDown::HandleDownTimeout,
		FMath::Max(DownTimeout, 0.0f),
		false);
}

void UPRGA_PlayerDown::StopDownTimer()
{
	UWorld* World = GetWorld();
	if (IsValid(World))
	{
		World->GetTimerManager().ClearTimer(DownTimerHandle);
	}
}

void UPRGA_PlayerDown::HandleDownTimeout()
{
	SendDownDeathConfirmedEvent();
}

void UPRGA_PlayerDown::SendDownDeathConfirmedEvent() const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor) || !AvatarActor->HasAuthority())
	{
		return;
	}

	const APRPlayerCharacter* PlayerCharacter = Cast<APRPlayerCharacter>(AvatarActor);
	APRPlayerState* PlayerState = IsValid(PlayerCharacter) ? PlayerCharacter->GetPlayerState<APRPlayerState>() : nullptr;
	if (IsValid(PlayerState))
	{
		PlayerState->SendSurvivalGameplayEvent(PRGameplayTags::Event_Ability_PlayerDeathConfirmed);
	}
}

void UPRGA_PlayerDown::StartDownDeathEventWait()
{
	if (IsValid(ActiveDownDeathEventTask))
	{
		return;
	}

	ActiveDownDeathEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		PRGameplayTags::Event_Ability_PlayerDeathConfirmed,
		nullptr,
		/*OnlyTriggerOnce=*/true,
		/*OnlyMatchExact=*/true);

	if (IsValid(ActiveDownDeathEventTask))
	{
		ActiveDownDeathEventTask->EventReceived.AddDynamic(this, &UPRGA_PlayerDown::HandleDownDeathEvent);
		ActiveDownDeathEventTask->ReadyForActivation();
	}
}

void UPRGA_PlayerDown::StopDownDeathEventWait()
{
	if (IsValid(ActiveDownDeathEventTask))
	{
		ActiveDownDeathEventTask->EndTask();
		ActiveDownDeathEventTask = nullptr;
	}
}

void UPRGA_PlayerDown::StartDownToDeath()
{
	if (bDownToDeathStarted)
	{
		return;
	}
	
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(ASC))
	{
		return;
	}

	ASC->AddLooseGameplayTag(PRGameplayTags::State_Block_Move);
	
	bDownToDeathStarted = true;
	StopDownTimer();
	StopDownDeathEventWait();
	SetDownEnterMovementBlocked(false);

	if (IsValid(ActiveMontageTask))
	{
		ActiveMontageTask->EndTask();
		ActiveMontageTask = nullptr;
	}

	if (!IsValid(DownToDeathMontage))
	{
		FinalizeDownDeath();
		return;
	}

	ActiveMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		DownToDeathMontage,
		FMath::Max(MontagePlayRate, UE_SMALL_NUMBER));
	if (!IsValid(ActiveMontageTask))
	{
		FinalizeDownDeath();
		return;
	}

	ActiveMontageTask->OnCompleted.AddDynamic(this, &UPRGA_PlayerDown::HandleDownToDeathMontageCompleted);
	ActiveMontageTask->OnBlendOut.AddDynamic(this, &UPRGA_PlayerDown::HandleDownToDeathMontageCompleted);
	ActiveMontageTask->OnInterrupted.AddDynamic(this, &UPRGA_PlayerDown::HandleDownToDeathMontageInterrupted);
	ActiveMontageTask->OnCancelled.AddDynamic(this, &UPRGA_PlayerDown::HandleDownToDeathMontageInterrupted);
	ActiveMontageTask->ReadyForActivation();
	StartDownToDeathFinalizeTimer();
}

void UPRGA_PlayerDown::StartDownToDeathFinalizeTimer()
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor) || !AvatarActor->HasAuthority() || !IsValid(DownToDeathMontage))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	const float FinalizeDelay = DownToDeathMontage->GetPlayLength() / FMath::Max(MontagePlayRate, UE_SMALL_NUMBER);
	World->GetTimerManager().SetTimer(
		DownToDeathFinalizeTimerHandle,
		this,
		&UPRGA_PlayerDown::HandleDownToDeathFinalizeTimeout,
		FMath::Max(FinalizeDelay, UE_SMALL_NUMBER),
		false);
}

void UPRGA_PlayerDown::StopDownToDeathFinalizeTimer()
{
	UWorld* World = GetWorld();
	if (IsValid(World))
	{
		World->GetTimerManager().ClearTimer(DownToDeathFinalizeTimerHandle);
	}
}

void UPRGA_PlayerDown::HandleDownToDeathFinalizeTimeout()
{
	FinalizeDownDeath();
}

void UPRGA_PlayerDown::SetDownEnterMovementBlocked(bool bBlocked)
{
	if (bDownEnterMovementBlockAdded == bBlocked)
	{
		return;
	}

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor) || !AvatarActor->HasAuthority())
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(ASC))
	{
		return;
	}

	if (bBlocked)
	{
		ASC->AddLooseGameplayTag(PRGameplayTags::State_Block_Move);
		ASC->AddReplicatedLooseGameplayTag(PRGameplayTags::State_Block_Move);
		bDownEnterMovementBlockAdded = true;
		return;
	}

	ASC->RemoveLooseGameplayTag(PRGameplayTags::State_Block_Move);
	ASC->RemoveReplicatedLooseGameplayTag(PRGameplayTags::State_Block_Move);
	bDownEnterMovementBlockAdded = false;
}

void UPRGA_PlayerDown::FinalizeDownDeath()
{
	if (bDownDeathFinalized)
	{
		return;
	}

	bDownDeathFinalized = true;
	ApplyDownDeathStateTags();

	if (APRPlayerCharacter* PlayerCharacter = Cast<APRPlayerCharacter>(GetAvatarActorFromActorInfo()))
	{
		UCharacterMovementComponent* CharacterMovement = PlayerCharacter->GetCharacterMovement();
		if (IsValid(CharacterMovement))
		{
			CharacterMovement->StopMovementImmediately();
			CharacterMovement->DisableMovement();
		}
	}

	NotifyDeathToGameMode();
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UPRGA_PlayerDown::ApplyDownDeathStateTags()
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor) || !AvatarActor->HasAuthority())
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
}

void UPRGA_PlayerDown::NotifyDownToGameMode() const
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

void UPRGA_PlayerDown::NotifyDeathToGameMode() const
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

void UPRGA_PlayerDown::PlayDownEnterMontage()
{
	if (!IsValid(DownEnterMontage))
	{
		return;
	}

	SetDownEnterMovementBlocked(true);

	if (IsValid(ActiveMontageTask))
	{
		ActiveMontageTask->EndTask();
		ActiveMontageTask = nullptr;
	}

	ActiveMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		DownEnterMontage,
		FMath::Max(MontagePlayRate, UE_SMALL_NUMBER));
	if (!IsValid(ActiveMontageTask))
	{
		SetDownEnterMovementBlocked(false);
		return;
	}

	ActiveMontageTask->OnCompleted.AddDynamic(this, &UPRGA_PlayerDown::HandleDownMontageCompleted);
	ActiveMontageTask->OnBlendOut.AddDynamic(this, &UPRGA_PlayerDown::HandleDownMontageCompleted);
	ActiveMontageTask->OnInterrupted.AddDynamic(this, &UPRGA_PlayerDown::HandleDownMontageInterrupted);
	ActiveMontageTask->OnCancelled.AddDynamic(this, &UPRGA_PlayerDown::HandleDownMontageInterrupted);
	ActiveMontageTask->ReadyForActivation();
}

// Copyright ProjectR. All Rights Reserved.

#include "PRGameplayAbility_EnemyDeath.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AIController.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ProjectR/PRGameplayTags.h"

UPRGameplayAbility_EnemyDeath::UPRGameplayAbility_EnemyDeath()
{
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	TriggerData.TriggerTag = PRGameplayTags::Event_Ability_Death;
	AbilityTriggers.Add(TriggerData);

	// 사망 Ability 활성화 시 GAS 내장 태그 취소 규약으로 진행 중인 패턴을 중단한다.
	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Enemy_Pattern);
	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Boss_Pattern);
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

	bDeathFinished = false;

	if (ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		if (bDisableMovementOnDeath)
		{
			// 사망 후에는 AI/루트모션/외부 이동이 다시 캐릭터를 움직이지 못하게 막는다.
			Character->GetCharacterMovement()->DisableMovement();
		}
		else
		{
			Character->GetCharacterMovement()->StopMovementImmediately();
		}

		if (AAIController* AIController = Cast<AAIController>(Character->GetController()))
		{
			AIController->StopMovement();
		}
	}

	if (IsValid(DeathMontage))
	{
		// 기본 정책은 사망 Ability를 끝내지 않는 것이다.
		// 몽타주는 재생하되 State.Dead와 Ability 상태는 유지해 사망 상태를 고정한다.
		ActiveMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			NAME_None,
			DeathMontage,
			FMath::Max(MontagePlayRate, UE_SMALL_NUMBER));

		if (IsValid(ActiveMontageTask))
		{
			ActiveMontageTask->OnCompleted.AddDynamic(this, &UPRGameplayAbility_EnemyDeath::HandleDeathMontageCompleted);
			ActiveMontageTask->OnBlendOut.AddDynamic(this, &UPRGameplayAbility_EnemyDeath::HandleDeathMontageCompleted);
			ActiveMontageTask->OnInterrupted.AddDynamic(this, &UPRGameplayAbility_EnemyDeath::HandleDeathMontageInterrupted);
			ActiveMontageTask->OnCancelled.AddDynamic(this, &UPRGameplayAbility_EnemyDeath::HandleDeathMontageInterrupted);
			ActiveMontageTask->ReadyForActivation();
		}
	}
}

void UPRGameplayAbility_EnemyDeath::EndAbility(const FGameplayAbilitySpecHandle Handle,
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

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UPRGameplayAbility_EnemyDeath::HandleDeathMontageCompleted()
{
	if (bEndAbilityWhenMontageEnds)
	{
		FinishDeath(false);
	}
}

void UPRGameplayAbility_EnemyDeath::HandleDeathMontageInterrupted()
{
	if (bEndAbilityWhenMontageEnds)
	{
		FinishDeath(true);
	}
}

void UPRGameplayAbility_EnemyDeath::FinishDeath(bool bWasCancelled)
{
	if (bDeathFinished)
	{
		return;
	}

	bDeathFinished = true;
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bWasCancelled);
}

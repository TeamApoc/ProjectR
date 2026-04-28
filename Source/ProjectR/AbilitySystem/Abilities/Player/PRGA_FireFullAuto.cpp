// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRGA_FireFullAuto.h"

#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "ProjectR/AbilitySystem/Tasks/PRAbilityTask_RepeatFire.h"

UPRGA_FireFullAuto::UPRGA_FireFullAuto()
{
}

void UPRGA_FireFullAuto::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/true);
		return;
	}

	// FireIntrval 주기로 FireOneShot 반복
	if (UPRAbilityTask_RepeatFire* RepeatTask = UPRAbilityTask_RepeatFire::RepeatFire(this, FireInterval, bFireOnActivate))
	{
		RepeatTask->OnPerform.AddDynamic(this, &UPRGA_FireFullAuto::FireOneShot);
		RepeatTask->ReadyForActivation();
	}

	// 인풋 릴리즈 시 종료
	if (UAbilityTask_WaitInputRelease* ReleaseTask = UAbilityTask_WaitInputRelease::WaitInputRelease(this))
	{
		ReleaseTask->OnRelease.AddDynamic(this, &UPRGA_FireFullAuto::HandleInputRelease);
		ReleaseTask->ReadyForActivation();
	}
}

void UPRGA_FireFullAuto::HandleInputRelease(float TimeHeld)
{
	ResetConsecutiveShots();
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/false);
}

// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRAbilityTask_RepeatFire.h"

#include "ProjectR/AbilitySystem/Abilities/Player/PRGA_Fire.h"

UPRAbilityTask_RepeatFire* UPRAbilityTask_RepeatFire::RepeatFire(UGameplayAbility* OwningAbility, float Interval,
                                                                 bool bFireImmediately)
{
	UPRAbilityTask_RepeatFire* Task = NewAbilityTask<UPRAbilityTask_RepeatFire>(OwningAbility);
	Task->Interval = Interval;
	Task->bFireImmediately = bFireImmediately;
	return Task; 
}

void UPRAbilityTask_RepeatFire::Activate()
{
	Super::Activate();
	
	if (bFireImmediately && ShouldBroadcastAbilityTaskDelegates())
	{
		OnPerform.Broadcast();
	}
	
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(TimerHandle, this, &UPRAbilityTask_RepeatFire::PerformFire, Interval, true);
	}
}

void UPRAbilityTask_RepeatFire::OnDestroy(bool bInOwnerFinished)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TimerHandle);
	}
	
	Super::OnDestroy(bInOwnerFinished);
}

void UPRAbilityTask_RepeatFire::PerformFire()
{
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnPerform.Broadcast();
	}
}

// Copyright ProjectR. All Rights Reserved.

#include "PRGameplayAbility_EnemyGroggy.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AIController.h"
#include "Animation/AnimMontage.h"
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

	// 그로기에 들어가면 현재 공격/패턴 Ability를 먼저 끊는다.
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

	bGroggyFinished = false;

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->CancelAbilities(&CancelAbilityTags, nullptr, this);
	}

	if (ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		// 이동 속도를 즉시 끊어 그로기 몽타주가 이동 입력과 섞이지 않게 한다.
		Character->GetCharacterMovement()->StopMovementImmediately();

		if (AAIController* AIController = Cast<AAIController>(Character->GetController()))
		{
			AIController->StopMovement();
		}
	}

	const bool bHasGroggyMontage = IsValid(GroggyMontage);
	if (bHasGroggyMontage)
	{
		// 몽타주가 끝나면 그로기도 끝낼 수 있도록 콜백을 연결한다.
		ActiveMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			NAME_None,
			GroggyMontage,
			FMath::Max(MontagePlayRate, UE_SMALL_NUMBER));

		if (IsValid(ActiveMontageTask))
		{
			ActiveMontageTask->OnCompleted.AddDynamic(this, &UPRGameplayAbility_EnemyGroggy::HandleGroggyMontageCompleted);
			ActiveMontageTask->OnBlendOut.AddDynamic(this, &UPRGameplayAbility_EnemyGroggy::HandleGroggyMontageCompleted);
			ActiveMontageTask->OnInterrupted.AddDynamic(this, &UPRGameplayAbility_EnemyGroggy::HandleGroggyMontageInterrupted);
			ActiveMontageTask->OnCancelled.AddDynamic(this, &UPRGameplayAbility_EnemyGroggy::HandleGroggyMontageInterrupted);
			ActiveMontageTask->ReadyForActivation();
		}
	}

	if (UWorld* World = GetWorld())
	{
		const float MontageDuration = bHasGroggyMontage && bEndWhenMontageEnds
			? GroggyMontage->GetPlayLength() / FMath::Max(MontagePlayRate, UE_SMALL_NUMBER)
			: 0.0f;
		// 최소 그로기 시간과 몽타주 길이 중 더 긴 값을 사용한다.
		const float FinishDelay = FMath::Max(GroggyDuration, MontageDuration + 0.1f);

		if (FinishDelay > 0.0f)
		{
			World->GetTimerManager().SetTimer(GroggyTimerHandle,
				FTimerDelegate::CreateUObject(this, &UPRGameplayAbility_EnemyGroggy::FinishGroggy, false),
				FinishDelay,
				false);
		}
	}
}

void UPRGameplayAbility_EnemyGroggy::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(GroggyTimerHandle);
	}

	if (IsValid(ActiveMontageTask))
	{
		ActiveMontageTask->EndTask();
		ActiveMontageTask = nullptr;
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		// 그로기 Ability가 끝나는 시점에 State.Groggy를 제거해 BT/Ability 차단 상태를 해제한다.
		ASC->RemoveLooseGameplayTag(PRGameplayTags::State_Groggy);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UPRGameplayAbility_EnemyGroggy::HandleGroggyMontageCompleted()
{
	if (bEndWhenMontageEnds)
	{
		FinishGroggy(false);
	}
}

void UPRGameplayAbility_EnemyGroggy::HandleGroggyMontageInterrupted()
{
	FinishGroggy(true);
}

void UPRGameplayAbility_EnemyGroggy::FinishGroggy(bool bWasCancelled)
{
	if (bGroggyFinished)
	{
		return;
	}

	bGroggyFinished = true;
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bWasCancelled);
}

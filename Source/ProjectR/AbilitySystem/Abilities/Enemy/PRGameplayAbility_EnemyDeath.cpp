// Copyright ProjectR. All Rights Reserved.

#include "PRGameplayAbility_EnemyDeath.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AIController.h"
#include "Animation/AnimMontage.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"
#include "ProjectR/Character/Enemy/PREnemyBaseCharacter.h"
#include "ProjectR/PRGameplayTags.h"

UPRGameplayAbility_EnemyDeath::UPRGameplayAbility_EnemyDeath()
{
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	TriggerData.TriggerTag = PRGameplayTags::Event_Ability_Death;
	AbilityTriggers.Add(TriggerData);

	// 사망 Ability 활성화 시 진행 중인 적/보스 패턴 Ability를 모두 중단하고 새 패턴 진입을 막는다.
	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Enemy_Pattern);
	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Boss_Pattern);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Enemy_Pattern);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Boss_Pattern);
	ActivationOwnedTags.AddTag(PRGameplayTags::State_Dead);
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
			// 사망 이후 AI, Root Motion, 관성 이동이 다시 캐릭터를 움직이지 못하게 잠근다.
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

		if (UCapsuleComponent* CapsuleComponent = Character->GetCapsuleComponent())
		{
			// 사망 연출 중 길막이나 추가 피격 판정이 남지 않도록 Pawn 충돌을 끈다.
			CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}

	if (IsValid(DeathMontage))
	{
		// 사망 몽타주는 다른 공격/그로기 몽타주와 동일하게 GAS 몽타주 복제 경로만 사용한다.
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

	if (bUseDissolveOnDeath)
	{
		RequestDeathDissolveVisual();
		ScheduleDeathDestroy(CalculateDeathDestroyDelay());
	}
	else
	{
		ScheduleDeathDestroy(DeathDestroyDelay);
	}
}

void UPRGameplayAbility_EnemyDeath::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DeathDestroyTimerHandle);
	}

	if (IsValid(ActiveMontageTask))
	{
		ActiveMontageTask->EndTask();
		ActiveMontageTask = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UPRGameplayAbility_EnemyDeath::HandleDeathMontageCompleted()
{
	if (!bUseDissolveOnDeath && bEndAbilityWhenMontageEnds)
	{
		FinishDeath();
	}
}

void UPRGameplayAbility_EnemyDeath::HandleDeathMontageInterrupted()
{
	if (!bUseDissolveOnDeath && bEndAbilityWhenMontageEnds)
	{
		FinishDeath();
	}
}

void UPRGameplayAbility_EnemyDeath::DestroyDeathAvatar()
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor) || !AvatarActor->HasAuthority())
	{
		return;
	}

	FinishDeath();
	AvatarActor->Destroy();
}

void UPRGameplayAbility_EnemyDeath::ScheduleDeathDestroy(float Delay)
{
	if (!bDestroyActorOnDeath)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			DeathDestroyTimerHandle,
			this,
			&UPRGameplayAbility_EnemyDeath::DestroyDeathAvatar,
			FMath::Max(Delay, 0.0f),
			false);
	}
	else
	{
		DestroyDeathAvatar();
	}
}

void UPRGameplayAbility_EnemyDeath::RequestDeathDissolveVisual()
{
	APREnemyBaseCharacter* EnemyCharacter = Cast<APREnemyBaseCharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(EnemyCharacter))
	{
		return;
	}

	EnemyCharacter->RequestDeathDissolveVisual(
		IsValid(ActiveMontageTask) ? DeathMontage.Get() : nullptr,
		MontagePlayRate,
		DissolveDelayAfterMontage,
		DissolveDuration,
		DissolveStartValue,
		DissolveEndValue,
		DissolveScalarParameterName,
		DissolveNiagaraSystem,
		NiagaraDissolveParameterName,
		DissolveTexture,
		DissolveTextureUV,
		DissolveTickInterval);
}

float UPRGameplayAbility_EnemyDeath::CalculateDeathDestroyDelay() const
{
	if (!bUseDissolveOnDeath)
	{
		return DeathDestroyDelay;
	}

	const float MontageDelay = IsValid(DeathMontage) && IsValid(ActiveMontageTask)
		? (DeathMontage->GetPlayLength() / FMath::Max(MontagePlayRate, UE_SMALL_NUMBER)) + 0.1f
		: 0.0f;
	const float StartDelay = FMath::Max(DissolveDelayAfterMontage, 0.0f);
	const float VisualDuration = FMath::Max(DissolveDuration, 0.0f);

	return MontageDelay + StartDelay + VisualDuration;
}

void UPRGameplayAbility_EnemyDeath::FinishDeath()
{
	if (bDeathFinished)
	{
		return;
	}

	bDeathFinished = true;
}

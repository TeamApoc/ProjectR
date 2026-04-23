// Copyright ProjectR. All Rights Reserved.

#include "PRGameplayAbility_EnemyMeleeAttack.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Animation/AnimMontage.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ProjectR/AI/Components/PREnemyThreatComponent.h"
#include "ProjectR/Character/Enemy/PREnemyBaseCharacter.h"
#include "ProjectR/Combat/PRCombatGameplayTags.h"
#include "ProjectR/Combat/PRCombatStatics.h"
#include "ProjectR/PRGameplayTags.h"

UPRGameplayAbility_EnemyMeleeAttack::UPRGameplayAbility_EnemyMeleeAttack()
{
	ActivationBlockedTags.AddTag(PRGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(PRGameplayTags::State_Groggy);
	ActivationBlockedTags.AddTag(PRGameplayTags::State_PhaseTransitioning);
}

void UPRGameplayAbility_EnemyMeleeAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor) || !AvatarActor->HasAuthority() || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	DamagedActors.Reset();
	bMeleeAttackFinished = false;
	bMeleeHitTriggered = false;

	if (ACharacter* SourceCharacter = Cast<ACharacter>(AvatarActor))
	{
		// 공격 시작 시 BT 이동 잔여 속도 제거
		SourceCharacter->GetCharacterMovement()->StopMovementImmediately();
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const bool bHasAttackMontage = IsValid(AttackMontage);
	if (bHasAttackMontage)
	{
		// Ability 종료와 몽타주 정리/네트워크 처리를 함께 다루는 GAS 몽타주 태스크
		ActiveMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			NAME_None,
			AttackMontage,
			FMath::Max(MontagePlayRate, UE_SMALL_NUMBER),
			MontageStartSection);

		if (IsValid(ActiveMontageTask))
		{
			ActiveMontageTask->OnCompleted.AddDynamic(this, &UPRGameplayAbility_EnemyMeleeAttack::HandleAttackMontageCompleted);
			ActiveMontageTask->OnBlendOut.AddDynamic(this, &UPRGameplayAbility_EnemyMeleeAttack::HandleAttackMontageCompleted);
			ActiveMontageTask->OnInterrupted.AddDynamic(this, &UPRGameplayAbility_EnemyMeleeAttack::HandleAttackMontageInterrupted);
			ActiveMontageTask->OnCancelled.AddDynamic(this, &UPRGameplayAbility_EnemyMeleeAttack::HandleAttackMontageInterrupted);
			ActiveMontageTask->ReadyForActivation();
		}
	}

	if (bUseAnimationNotifyForHit)
	{
		// AnimNotify는 GameplayEvent만 발행
		// 활성 Ability 인스턴스에서 이벤트를 받아 서버 판정 실행
		ActiveMeleeHitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this,
			PRCombatGameplayTags::Event_Ability_EnemyMeleeHit,
			nullptr,
			true,
			true);

		if (IsValid(ActiveMeleeHitEventTask))
		{
			ActiveMeleeHitEventTask->EventReceived.AddDynamic(this, &UPRGameplayAbility_EnemyMeleeAttack::HandleMeleeHitGameplayEvent);
			ActiveMeleeHitEventTask->ReadyForActivation();
		}
	}

	const bool bUseTimedHit = !bHasAttackMontage
		|| !bUseAnimationNotifyForHit
		|| bAllowTimedHitFallbackWhenMontagePlays;

	// 몽타주+Notify 공격은 Notify 타격 프레임 우선
	// 몽타주 없음 또는 fallback 허용 시 WindupTime 타이머 사용
	if (bUseTimedHit && WindupTime <= 0.0f)
	{
		TriggerMeleeHitOnce();
	}
	else if (bUseTimedHit)
	{
		World->GetTimerManager().SetTimer(HitTimerHandle, this,
			&UPRGameplayAbility_EnemyMeleeAttack::TriggerMeleeHitOnce, WindupTime, false);
	}

	const float TimerFinishDelay = FMath::Max(WindupTime + RecoveryTime, 0.01f);
	const float MontageFinishDelay = bHasAttackMontage && bUseMontageDurationForFinish
		? (AttackMontage->GetPlayLength() / FMath::Max(MontagePlayRate, UE_SMALL_NUMBER)) + 0.1f
		: 0.0f;
	// 몽타주 콜백 누락 대비 종료 타이머
	const float FinishDelay = FMath::Max(TimerFinishDelay, MontageFinishDelay);
	World->GetTimerManager().SetTimer(FinishTimerHandle, this,
		&UPRGameplayAbility_EnemyMeleeAttack::FinishMeleeAttack, FinishDelay, false);
}

void UPRGameplayAbility_EnemyMeleeAttack::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HitTimerHandle);
		World->GetTimerManager().ClearTimer(FinishTimerHandle);
	}

	if (IsValid(ActiveMontageTask))
	{
		ActiveMontageTask->EndTask();
		ActiveMontageTask = nullptr;
	}

	if (IsValid(ActiveMeleeHitEventTask))
	{
		ActiveMeleeHitEventTask->EndTask();
		ActiveMeleeHitEventTask = nullptr;
	}

	DamagedActors.Reset();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UPRGameplayAbility_EnemyMeleeAttack::TriggerMeleeHitFromAnimation()
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor) || !AvatarActor->HasAuthority())
	{
		return;
	}

	TriggerMeleeHitOnce();
}

void UPRGameplayAbility_EnemyMeleeAttack::HandleMeleeHitGameplayEvent(FGameplayEventData Payload)
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor)
		|| !AvatarActor->HasAuthority()
		|| Payload.EventTag != PRCombatGameplayTags::Event_Ability_EnemyMeleeHit)
	{
		return;
	}

	TriggerMeleeHitOnce();
}

void UPRGameplayAbility_EnemyMeleeAttack::HandleAttackMontageCompleted()
{
	if (bUseMontageDurationForFinish)
	{
		FinishMeleeAttack();
	}
}

void UPRGameplayAbility_EnemyMeleeAttack::HandleAttackMontageInterrupted()
{
	if (bMeleeAttackFinished)
	{
		return;
	}

	bMeleeAttackFinished = true;
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UPRGameplayAbility_EnemyMeleeAttack::TriggerMeleeHitOnce()
{
	if (bMeleeAttackFinished || bMeleeHitTriggered)
	{
		return;
	}

	bMeleeHitTriggered = true;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HitTimerHandle);
	}

	ExecuteMeleeHit();
}

void UPRGameplayAbility_EnemyMeleeAttack::ExecuteMeleeHit()
{
	ACharacter* SourceCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(SourceCharacter) || !SourceCharacter->HasAuthority())
	{
		return;
	}

	ApplyForwardLunge(SourceCharacter);

	const FVector Forward = SourceCharacter->GetActorForwardVector();
	const FVector TraceStart = SourceCharacter->GetActorLocation() + FVector(0.0f, 0.0f, TraceHeightOffset);
	const FVector TraceEnd = TraceStart + Forward * AttackRange;

	// 현재 판정 기준은 무기 소켓이 아닌 캐릭터 정면 구체 Sweep
	// 무기 궤적 판정 전환 시 교체 지점
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PREnemyMeleeAttack), false, SourceCharacter);
	QueryParams.AddIgnoredActor(SourceCharacter);

	TArray<FHitResult> HitResults;
	const FCollisionShape CollisionShape = FCollisionShape::MakeSphere(AttackRadius);
	const bool bHit = SourceCharacter->GetWorld()->SweepMultiByChannel(
		HitResults,
		TraceStart,
		TraceEnd,
		FQuat::Identity,
		TraceChannel,
		CollisionShape,
		QueryParams);

	if (bDrawDebug)
	{
		const FColor DebugColor = bHit ? FColor::Red : FColor::Green;
		DrawDebugLine(SourceCharacter->GetWorld(), TraceStart, TraceEnd, DebugColor, false, 1.0f, 0, 2.0f);
		DrawDebugSphere(SourceCharacter->GetWorld(), TraceEnd, AttackRadius, 16, DebugColor, false, 1.0f);
	}

	for (const FHitResult& HitResult : HitResults)
	{
		AActor* HitActor = HitResult.GetActor();
		if (!ShouldDamageActor(HitActor) || DamagedActors.Contains(HitActor))
		{
			continue;
		}

		FPRDamageContext DamageContext;
		DamageContext.SourceActor = SourceCharacter;
		DamageContext.TargetActor = HitActor;
		DamageContext.EffectCauser = SourceCharacter;
		DamageContext.SourceObject = this;
		DamageContext.HitResult = HitResult;
		DamageContext.SourceAbilityTag = AbilityTag;
		DamageContext.Damage = Damage;
		DamageContext.GroggyDamage = GroggyDamage;
		DamageContext.AbilityLevel = GetAbilityLevel();

		// 공용 Damage GE와 SetByCaller 값 변환은 ApplyDamageContext 내부 처리
		if (ApplyDamageContext(DamageContext))
		{
			DamagedActors.Add(HitActor);
		}
	}
}

void UPRGameplayAbility_EnemyMeleeAttack::FinishMeleeAttack()
{
	if (bMeleeAttackFinished)
	{
		return;
	}

	bMeleeAttackFinished = true;
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UPRGameplayAbility_EnemyMeleeAttack::ApplyForwardLunge(ACharacter* SourceCharacter) const
{
	if (!bUseForwardLunge || !IsValid(SourceCharacter) || LungeDistance <= 0.0f)
	{
		return;
	}

	const FVector MoveDelta = SourceCharacter->GetActorForwardVector() * LungeDistance;
	FHitResult SweepHit;
	// 벽 관통 방지용 Sweep 이동
	SourceCharacter->AddActorWorldOffset(MoveDelta, true, &SweepHit);
}

bool UPRGameplayAbility_EnemyMeleeAttack::ShouldDamageActor(const AActor* CandidateActor) const
{
	if (!IsValid(CandidateActor) || CandidateActor == GetAvatarActorFromActorInfo())
	{
		return false;
	}

	if (!IsValid(UPRCombatStatics::FindAbilitySystemComponent(CandidateActor)))
	{
		return false;
	}

	if (!bOnlyDamageThreatTarget)
	{
		return true;
	}

	const APREnemyBaseCharacter* EnemyCharacter = GetEnemyAvatarCharacter();
	const UPREnemyThreatComponent* ThreatComponent = IsValid(EnemyCharacter)
		? EnemyCharacter->GetEnemyThreatComponent()
		: nullptr;

	// 기본 판정 대상은 현재 ThreatTarget
	// 주변 플레이어 광역 공격은 옵션 해제 후 별도 패턴 구성
	return IsValid(ThreatComponent) && ThreatComponent->GetCurrentTarget() == CandidateActor;
}

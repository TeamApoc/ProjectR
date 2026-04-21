// Copyright ProjectR. All Rights Reserved.

#include "PRGameplayAbility_EnemyMeleeAttack.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimMontage.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ProjectR/AI/Components/PREnemyThreatComponent.h"
#include "ProjectR/Character/Enemy/PREnemyBaseCharacter.h"
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
		// 공격이 시작되면 BT 이동이 남긴 속도를 끊고 몽타주/루트모션이 위치를 주도하게 한다.
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
		// GAS 몽타주 태스크를 사용하면 Ability 종료 시 몽타주 정리와 네트워크 처리가 함께 따라온다.
		ActiveMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			NAME_None,
			AttackMontage,
			FMath::Max(MontagePlayRate, UE_SMALL_NUMBER));

		if (IsValid(ActiveMontageTask))
		{
			ActiveMontageTask->OnCompleted.AddDynamic(this, &UPRGameplayAbility_EnemyMeleeAttack::HandleAttackMontageCompleted);
			ActiveMontageTask->OnBlendOut.AddDynamic(this, &UPRGameplayAbility_EnemyMeleeAttack::HandleAttackMontageCompleted);
			ActiveMontageTask->OnInterrupted.AddDynamic(this, &UPRGameplayAbility_EnemyMeleeAttack::HandleAttackMontageInterrupted);
			ActiveMontageTask->OnCancelled.AddDynamic(this, &UPRGameplayAbility_EnemyMeleeAttack::HandleAttackMontageInterrupted);
			ActiveMontageTask->ReadyForActivation();
		}
	}

	const bool bUseTimedHit = !bHasAttackMontage
		|| !bUseAnimationNotifyForHit
		|| bAllowTimedHitFallbackWhenMontagePlays;

	// 몽타주+Notify 공격은 Notify가 타격 프레임을 결정한다.
	// 몽타주가 없거나 fallback을 켠 경우에만 WindupTime 타이머를 사용한다.
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
	// 몽타주 콜백이 누락되어도 Ability가 영원히 끝나지 않도록 타이머를 함께 둔다.
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

	// 현재 구현은 무기 소켓 기반이 아니라 캐릭터 정면 구체 Sweep이다.
	// 무기 궤적 기반 판정이 필요해지면 이 지점을 교체하면 된다.
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

		// ApplyDamageContext 내부에서 공용 Damage GE와 SetByCaller 값으로 변환된다.
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
	// Sweep을 켜서 벽을 뚫고 이동하지 않도록 한다.
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

	// 기본값은 현재 ThreatTarget만 맞히는 것이다. 주변 플레이어 광역 공격은 옵션을 끄고 별도 패턴으로 구성한다.
	return IsValid(ThreatComponent) && ThreatComponent->GetCurrentTarget() == CandidateActor;
}

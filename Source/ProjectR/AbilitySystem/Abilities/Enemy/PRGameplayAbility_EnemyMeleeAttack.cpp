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
	bMeleeHitWindowActive = false;
	bHasPreviousMeleeWindowTracePoint = false;
	PreviousMeleeWindowTracePoint = FVector::ZeroVector;

	if (ACharacter* SourceCharacter = Cast<ACharacter>(AvatarActor))
	{
		// 공격 시작 시 BT 이동 잔여 속도 제거
		SourceCharacter->GetCharacterMovement()->StopMovementImmediately();
	}

	RefreshAttackFacing(bFaceTargetOnAbilityStart);

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
		const bool bOnlyTriggerOnce = !bAllowMultipleMeleeHits;
		constexpr bool bOnlyMatchExact = true;

		// 한 프레임 타격 Notify 이벤트
		ActiveMeleeHitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this,
			PRCombatGameplayTags::Event_Ability_EnemyMeleeHit,
			nullptr,
			bOnlyTriggerOnce,
			bOnlyMatchExact);

		if (IsValid(ActiveMeleeHitEventTask))
		{
			ActiveMeleeHitEventTask->EventReceived.AddDynamic(this, &UPRGameplayAbility_EnemyMeleeAttack::HandleMeleeHitGameplayEvent);
			ActiveMeleeHitEventTask->ReadyForActivation();
		}

		// 구간 판정 시작 이벤트
		ActiveMeleeHitWindowBeginEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this,
			PRCombatGameplayTags::Event_Ability_EnemyMeleeWindowBegin,
			nullptr,
			false,
			bOnlyMatchExact);

		if (IsValid(ActiveMeleeHitWindowBeginEventTask))
		{
			ActiveMeleeHitWindowBeginEventTask->EventReceived.AddDynamic(this, &UPRGameplayAbility_EnemyMeleeAttack::HandleMeleeHitWindowBeginGameplayEvent);
			ActiveMeleeHitWindowBeginEventTask->ReadyForActivation();
		}

		// 구간 판정 갱신 이벤트
		ActiveMeleeHitWindowTickEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this,
			PRCombatGameplayTags::Event_Ability_EnemyMeleeWindowTick,
			nullptr,
			false,
			bOnlyMatchExact);

		if (IsValid(ActiveMeleeHitWindowTickEventTask))
		{
			ActiveMeleeHitWindowTickEventTask->EventReceived.AddDynamic(this, &UPRGameplayAbility_EnemyMeleeAttack::HandleMeleeHitWindowTickGameplayEvent);
			ActiveMeleeHitWindowTickEventTask->ReadyForActivation();
		}

		// 구간 판정 종료 이벤트
		ActiveMeleeHitWindowEndEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this,
			PRCombatGameplayTags::Event_Ability_EnemyMeleeWindowEnd,
			nullptr,
			false,
			bOnlyMatchExact);

		if (IsValid(ActiveMeleeHitWindowEndEventTask))
		{
			ActiveMeleeHitWindowEndEventTask->EventReceived.AddDynamic(this, &UPRGameplayAbility_EnemyMeleeAttack::HandleMeleeHitWindowEndGameplayEvent);
			ActiveMeleeHitWindowEndEventTask->ReadyForActivation();
		}
	}

	const bool bUseTimedHit = !bHasAttackMontage
		|| !bUseAnimationNotifyForHit
		|| bAllowTimedHitFallbackWhenMontagePlays;

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

	if (IsValid(ActiveMeleeHitWindowBeginEventTask))
	{
		ActiveMeleeHitWindowBeginEventTask->EndTask();
		ActiveMeleeHitWindowBeginEventTask = nullptr;
	}

	if (IsValid(ActiveMeleeHitWindowTickEventTask))
	{
		ActiveMeleeHitWindowTickEventTask->EndTask();
		ActiveMeleeHitWindowTickEventTask = nullptr;
	}

	if (IsValid(ActiveMeleeHitWindowEndEventTask))
	{
		ActiveMeleeHitWindowEndEventTask->EndTask();
		ActiveMeleeHitWindowEndEventTask = nullptr;
	}

	DamagedActors.Reset();
	bMeleeHitWindowActive = false;
	bHasPreviousMeleeWindowTracePoint = false;
	PreviousMeleeWindowTracePoint = FVector::ZeroVector;

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

	if (bMeleeHitWindowActive)
	{
		return;
	}

	TriggerMeleeHitOnce();
}

void UPRGameplayAbility_EnemyMeleeAttack::HandleMeleeHitWindowBeginGameplayEvent(FGameplayEventData Payload)
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor)
		|| !AvatarActor->HasAuthority()
		|| Payload.EventTag != PRCombatGameplayTags::Event_Ability_EnemyMeleeWindowBegin)
	{
		return;
	}

	BeginMeleeHitWindow();
}

void UPRGameplayAbility_EnemyMeleeAttack::HandleMeleeHitWindowTickGameplayEvent(FGameplayEventData Payload)
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor)
		|| !AvatarActor->HasAuthority()
		|| Payload.EventTag != PRCombatGameplayTags::Event_Ability_EnemyMeleeWindowTick)
	{
		return;
	}

	UpdateMeleeHitWindow();
}

void UPRGameplayAbility_EnemyMeleeAttack::HandleMeleeHitWindowEndGameplayEvent(FGameplayEventData Payload)
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor)
		|| !AvatarActor->HasAuthority()
		|| Payload.EventTag != PRCombatGameplayTags::Event_Ability_EnemyMeleeWindowEnd)
	{
		return;
	}

	EndMeleeHitWindow();
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
	if (bMeleeAttackFinished || (!bAllowMultipleMeleeHits && bMeleeHitTriggered))
	{
		return;
	}

	bMeleeHitTriggered = true;
	if (bAllowMultipleMeleeHits)
	{
		DamagedActors.Reset();
	}

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

	RefreshAttackFacing(bFaceTargetOnMeleeHit);
	ApplyForwardLunge(SourceCharacter);

	FVector TracePoint = FVector::ZeroVector;
	if (GetCurrentAttackTracePoint(TracePoint))
	{
		ExecuteMeleeTrace(TracePoint, TracePoint);
		return;
	}

	const FVector Forward = SourceCharacter->GetActorForwardVector();
	const FVector TraceStart = SourceCharacter->GetActorLocation() + FVector(0.0f, 0.0f, TraceHeightOffset);
	const FVector TraceEnd = TraceStart + Forward * AttackRange;
	ExecuteMeleeTrace(TraceStart, TraceEnd);
}

void UPRGameplayAbility_EnemyMeleeAttack::BeginMeleeHitWindow()
{
	if (bMeleeAttackFinished)
	{
		return;
	}

	bMeleeHitWindowActive = true;
	bHasPreviousMeleeWindowTracePoint = false;
	PreviousMeleeWindowTracePoint = FVector::ZeroVector;
	DamagedActors.Reset();
	RefreshAttackFacing(bFaceTargetOnMeleeHit);
}

void UPRGameplayAbility_EnemyMeleeAttack::UpdateMeleeHitWindow()
{
	if (bMeleeAttackFinished || !bMeleeHitWindowActive)
	{
		return;
	}

	FVector CurrentTracePoint = FVector::ZeroVector;
	if (!GetCurrentAttackTracePoint(CurrentTracePoint))
	{
		ACharacter* SourceCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo());
		if (!IsValid(SourceCharacter) || !SourceCharacter->HasAuthority())
		{
			return;
		}

		const FVector TraceStart = SourceCharacter->GetActorLocation() + FVector(0.0f, 0.0f, TraceHeightOffset);
		const FVector TraceEnd = TraceStart + (SourceCharacter->GetActorForwardVector() * AttackRange);
		ExecuteMeleeTrace(TraceStart, TraceEnd);
		return;
	}

	if (!bHasPreviousMeleeWindowTracePoint)
	{
		ExecuteMeleeTrace(CurrentTracePoint, CurrentTracePoint);
		bHasPreviousMeleeWindowTracePoint = true;
		PreviousMeleeWindowTracePoint = CurrentTracePoint;
		return;
	}

	ExecuteMeleeTrace(PreviousMeleeWindowTracePoint, CurrentTracePoint);
	PreviousMeleeWindowTracePoint = CurrentTracePoint;
}

void UPRGameplayAbility_EnemyMeleeAttack::EndMeleeHitWindow()
{
	bMeleeHitWindowActive = false;
	bHasPreviousMeleeWindowTracePoint = false;
	PreviousMeleeWindowTracePoint = FVector::ZeroVector;
}

void UPRGameplayAbility_EnemyMeleeAttack::ExecuteMeleeTrace(const FVector& TraceStart, const FVector& TraceEnd)
{
	ACharacter* SourceCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(SourceCharacter) || !SourceCharacter->HasAuthority())
	{
		return;
	}

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
		DrawDebugSphere(SourceCharacter->GetWorld(), TraceStart, AttackRadius, 16, DebugColor, false, 1.0f);
		DrawDebugSphere(SourceCharacter->GetWorld(), TraceEnd, AttackRadius, 16, DebugColor, false, 1.0f);
	}

	for (const FHitResult& HitResult : HitResults)
	{
		AActor* HitActor = HitResult.GetActor();
		if (!ShouldDamageActor(HitActor) || DamagedActors.Contains(HitActor))
		{
			continue;
		}
		// 26.04.29, Yuchan, 아래 데미지 적용 코드는 ApplyDamage 함수로 대체됨.
		// FPRDamageContext DamageContext;
		// DamageContext.SourceActor = SourceCharacter;
		// DamageContext.TargetActor = HitActor;
		// DamageContext.EffectCauser = SourceCharacter;
		// DamageContext.SourceObject = this;
		// DamageContext.HitResult = HitResult;
		// DamageContext.SourceAbilityTag = AbilityTag;
		// DamageContext.Damage = Damage;
		// DamageContext.GroggyDamage = GroggyDamage;
		// DamageContext.AbilityLevel = GetAbilityLevel();
		//
		// if (ApplyDamageContext(DamageContext))
		// {
		// 	DamagedActors.Add(HitActor);
		// }
		
		ApplyDamage(HitActor, 1.0f, &HitResult); // TODO: 각 모션 별 AttackMultiplier 실제 값 전달이 필요 
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
	SourceCharacter->AddActorWorldOffset(MoveDelta, true, &SweepHit);
}

AActor* UPRGameplayAbility_EnemyMeleeAttack::GetCurrentThreatTarget() const
{
	const APREnemyBaseCharacter* EnemyCharacter = GetEnemyAvatarCharacter();
	const UPREnemyThreatComponent* ThreatComponent = IsValid(EnemyCharacter)
		? EnemyCharacter->GetEnemyThreatComponent()
		: nullptr;

	return IsValid(ThreatComponent)
		? ThreatComponent->GetCurrentTarget()
		: nullptr;
}

void UPRGameplayAbility_EnemyMeleeAttack::RefreshAttackFacing(bool bApplyActorRotation) const
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	AActor* TargetActor = GetCurrentThreatTarget();
	if (!IsValid(AvatarActor) || !IsValid(TargetActor))
	{
		return;
	}

	const FVector AvatarLocation = AvatarActor->GetActorLocation();
	const FVector TargetLocation = TargetActor->GetActorLocation();
	FVector DirectionToTarget = TargetLocation - AvatarLocation;
	DirectionToTarget.Z = 0.0f;
	if (DirectionToTarget.IsNearlyZero())
	{
		return;
	}

	FRotator FacingRotation = DirectionToTarget.Rotation();
	FacingRotation.Pitch = 0.0f;
	FacingRotation.Roll = 0.0f;

	if (bApplyActorRotation)
	{
		AvatarActor->SetActorRotation(FacingRotation);
	}
}

bool UPRGameplayAbility_EnemyMeleeAttack::GetCurrentAttackTracePoint(FVector& OutTracePoint) const
{
	const ACharacter* SourceCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(SourceCharacter))
	{
		return false;
	}

	const USkeletalMeshComponent* MeshComp = SourceCharacter->GetMesh();
	if (!IsValid(MeshComp) || AttackTraceSourceName == NAME_None)
	{
		return false;
	}

	const bool bHasSocket = MeshComp->DoesSocketExist(AttackTraceSourceName);
	const bool bHasBone = MeshComp->GetBoneIndex(AttackTraceSourceName) != INDEX_NONE;
	if (!bHasSocket && !bHasBone)
	{
		return false;
	}

	const FTransform SourceTransform = MeshComp->GetSocketTransform(AttackTraceSourceName, RTS_World);
	const FVector SourceLocation = SourceTransform.GetLocation();
	const FVector WorldOffset = SourceTransform.TransformVectorNoScale(AttackTraceSourceOffset);

	OutTracePoint = SourceLocation + WorldOffset;
	return true;
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

	return GetCurrentThreatTarget() == CandidateActor;
}

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

	return IsValid(ThreatComponent) && ThreatComponent->GetCurrentTarget() == CandidateActor;
}

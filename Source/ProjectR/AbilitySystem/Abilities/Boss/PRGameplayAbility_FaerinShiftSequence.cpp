// Copyright ProjectR. All Rights Reserved.

#include "PRGameplayAbility_FaerinShiftSequence.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimMontage.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ProjectR/AI/PREnemyEQSSelectionUtils.h"
#include "ProjectR/Character/Enemy/PRBossBaseCharacter.h"
#include "ProjectR/Character/Enemy/Boss/PRFaerinCharacterEventRouterComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogPRFaerinShiftSequence, Log, All);

UPRGameplayAbility_FaerinShiftSequence::UPRGameplayAbility_FaerinShiftSequence()
{
}

void UPRGameplayAbility_FaerinShiftSequence::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	APRBossBaseCharacter* BossCharacter = GetBossAvatarCharacter();
	if (!IsValid(BossCharacter) || !BossCharacter->HasAuthority() || !CanRunBossPattern())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	bShiftApplied = false;
	bShiftSequenceFinished = false;
	if (!RegisterCharacterEventListener() && bRequireCharacterEventForShift)
	{
		UE_LOG(LogPRFaerinShiftSequence, Warning,
			TEXT("Faerin shift cannot wait for CharacterEvent because router is missing. Ability=%s, Event=%s"),
			*GetNameSafe(this),
			*ShiftCharacterEventName.ToString());
		FinishShiftSequence(true);
		return;
	}

	if (!IsValid(ShiftMontage))
	{
		if (!bRequireCharacterEventForShift)
		{
			FVector Destination = FVector::ZeroVector;
			if (ResolveShiftDestination(Destination))
			{
				bShiftApplied = ApplyShiftToTarget(Destination);
			}
		}

		FinishShiftSequence(!bShiftApplied);
		return;
	}

	ActiveShiftMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		ShiftMontage,
		FMath::Max(ShiftMontagePlayRate, UE_SMALL_NUMBER),
		ShiftMontageStartSection);

	if (!IsValid(ActiveShiftMontageTask))
	{
		FinishShiftSequence(true);
		return;
	}

	ActiveShiftMontageTask->OnCompleted.AddDynamic(this, &UPRGameplayAbility_FaerinShiftSequence::HandleShiftMontageCompleted);
	ActiveShiftMontageTask->OnBlendOut.AddDynamic(this, &UPRGameplayAbility_FaerinShiftSequence::HandleShiftMontageBlendOut);
	ActiveShiftMontageTask->OnInterrupted.AddDynamic(this, &UPRGameplayAbility_FaerinShiftSequence::HandleShiftMontageInterrupted);
	ActiveShiftMontageTask->OnCancelled.AddDynamic(this, &UPRGameplayAbility_FaerinShiftSequence::HandleShiftMontageInterrupted);
	ActiveShiftMontageTask->ReadyForActivation();
}

void UPRGameplayAbility_FaerinShiftSequence::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	UnregisterCharacterEventListener();

	if (IsValid(ActiveShiftMontageTask))
	{
		ActiveShiftMontageTask->EndTask();
		ActiveShiftMontageTask = nullptr;
	}

	bShiftApplied = false;
	bShiftSequenceFinished = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UPRGameplayAbility_FaerinShiftSequence::RegisterCharacterEventListener()
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor))
	{
		return false;
	}

	ActiveEventRouter = AvatarActor->FindComponentByClass<UPRFaerinCharacterEventRouterComponent>();
	if (!IsValid(ActiveEventRouter))
	{
		return false;
	}

	ActiveEventRouter->RegisterFaerinEventListener(
		this,
		FFaerinCharacterEventSignature::FDelegate::CreateUObject(
			this,
			&UPRGameplayAbility_FaerinShiftSequence::HandleFaerinCharacterEvent));
	return true;
}

void UPRGameplayAbility_FaerinShiftSequence::UnregisterCharacterEventListener()
{
	if (IsValid(ActiveEventRouter))
	{
		ActiveEventRouter->UnregisterFaerinEventListener(this);
		ActiveEventRouter = nullptr;
	}
}

void UPRGameplayAbility_FaerinShiftSequence::HandleFaerinCharacterEvent(FName EventName)
{
	if (bShiftApplied || EventName != ShiftCharacterEventName)
	{
		return;
	}

	FVector Destination = FVector::ZeroVector;
	if (!ResolveShiftDestination(Destination))
	{
		UE_LOG(LogPRFaerinShiftSequence, Warning,
			TEXT("Faerin shift destination failed. Ability=%s, Event=%s"),
			*GetNameSafe(this),
			*EventName.ToString());
		FinishShiftSequence(true);
		return;
	}

	bShiftApplied = ApplyShiftToTarget(Destination);
	if (!bShiftApplied)
	{
		FinishShiftSequence(true);
	}
}

bool UPRGameplayAbility_FaerinShiftSequence::ResolveShiftDestination(FVector& OutDestination) const
{
	bool bResolved = false;

	if (DestinationMode == EPRFaerinShiftDestinationMode::EnvQuery)
	{
		bResolved = ResolveQueryShiftDestination(OutDestination);
		if (!bResolved && bFallbackToDirectionalDestinationWhenQueryFails)
		{
			bResolved = ResolveDirectionalShiftDestination(OutDestination);
		}
	}
	else
	{
		bResolved = ResolveDirectionalShiftDestination(OutDestination);
	}

	if (!bResolved)
	{
		return false;
	}

	if (bProjectDestinationToGround)
	{
		FVector ProjectedDestination = OutDestination;
		if (ProjectShiftDestinationToGround(OutDestination, ProjectedDestination))
		{
			OutDestination = ProjectedDestination;
		}
		else if (bRequireGroundProjection)
		{
			return false;
		}
	}

	return true;
}

bool UPRGameplayAbility_FaerinShiftSequence::ResolveQueryShiftDestination(FVector& OutDestination) const
{
	APRBossBaseCharacter* BossCharacter = GetBossAvatarCharacter();
	if (!IsValid(BossCharacter) || !IsValid(ShiftDestinationQuery.Get()))
	{
		return false;
	}

	return PREnemyEQSSelectionUtils::RunLocationQuery(
		BossCharacter,
		ShiftDestinationQuery.Get(),
		FloatParams,
		ShiftQueryRunMode,
		CandidateSelectionMode,
		TopCandidateCount,
		TopScoreCandidateRatio,
		OutDestination);
}

bool UPRGameplayAbility_FaerinShiftSequence::ResolveDirectionalShiftDestination(FVector& OutDestination) const
{
	const APRBossBaseCharacter* BossCharacter = GetBossAvatarCharacter();
	const AActor* TargetActor = GetBossPatternTarget();
	if (!IsValid(BossCharacter) || !IsValid(TargetActor))
	{
		return false;
	}

	FVector Direction = TargetActor->GetActorLocation() - BossCharacter->GetActorLocation();
	Direction.Z = 0.0f;
	if (!Direction.Normalize())
	{
		Direction = BossCharacter->GetActorForwardVector();
		Direction.Z = 0.0f;
		Direction.Normalize();
	}

	OutDestination = BossCharacter->GetActorLocation() + Direction * DirectionalDistanceFromBoss;
	OutDestination.Z = TargetActor->GetActorLocation().Z;
	return true;
}

bool UPRGameplayAbility_FaerinShiftSequence::ProjectShiftDestinationToGround(
	const FVector& InDestination,
	FVector& OutDestination) const
{
	const UWorld* World = GetWorld();
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(World) || !IsValid(AvatarActor))
	{
		return false;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(FaerinShiftGroundTrace), false, AvatarActor);
	const FVector TraceStart = InDestination + FVector(0.0f, 0.0f, GroundTraceUpOffset);
	const FVector TraceEnd = InDestination - FVector(0.0f, 0.0f, GroundTraceDownOffset);

	FHitResult HitResult;
	if (!World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, GroundTraceChannel, QueryParams))
	{
		return false;
	}

	OutDestination = HitResult.ImpactPoint + GroundLocationOffset;
	return true;
}

bool UPRGameplayAbility_FaerinShiftSequence::ApplyShiftToTarget(const FVector& Destination)
{
	APRBossBaseCharacter* BossCharacter = GetBossAvatarCharacter();
	AActor* TargetActor = GetBossPatternTarget();
	if (!IsValid(BossCharacter) || !IsValid(TargetActor))
	{
		return false;
	}

	FRotator TargetRotation = TargetActor->GetActorRotation();
	if (bFaceBossAfterShift)
	{
		const FVector ToBoss = BossCharacter->GetActorLocation() - Destination;
		TargetRotation = ToBoss.Rotation();
		TargetRotation.Pitch = 0.0f;
		TargetRotation.Roll = 0.0f;
	}

	FHitResult SweepHit;
	const bool bMoved = TargetActor->SetActorLocationAndRotation(
		Destination,
		TargetRotation,
		bSweepTargetMove,
		bSweepTargetMove ? &SweepHit : nullptr,
		ETeleportType::TeleportPhysics);

	if (!bMoved)
	{
		return false;
	}

	if (bStopTargetMovementAfterShift)
	{
		if (ACharacter* TargetCharacter = Cast<ACharacter>(TargetActor))
		{
			if (UCharacterMovementComponent* MovementComponent = TargetCharacter->GetCharacterMovement())
			{
				MovementComponent->StopMovementImmediately();
			}
		}
	}

	if (bDrawDebug)
	{
		DrawDebugSphere(GetWorld(), Destination, 60.0f, 16, FColor::Cyan, false, DebugDrawDuration);
	}

	return true;
}

void UPRGameplayAbility_FaerinShiftSequence::FinishShiftSequence(bool bWasCancelled)
{
	if (bShiftSequenceFinished)
	{
		return;
	}

	bShiftSequenceFinished = true;

	if (IsValid(ActiveShiftMontageTask))
	{
		ActiveShiftMontageTask->EndTask();
		ActiveShiftMontageTask = nullptr;
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bWasCancelled);
}

void UPRGameplayAbility_FaerinShiftSequence::HandleShiftMontageCompleted()
{
	if (bRequireCharacterEventForShift && !bShiftApplied)
	{
		FinishShiftSequence(true);
		return;
	}

	FinishShiftSequence(false);
}

void UPRGameplayAbility_FaerinShiftSequence::HandleShiftMontageBlendOut()
{
	if (bRequireCharacterEventForShift && !bShiftApplied)
	{
		FinishShiftSequence(true);
		return;
	}

	FinishShiftSequence(false);
}

void UPRGameplayAbility_FaerinShiftSequence::HandleShiftMontageInterrupted()
{
	FinishShiftSequence(true);
}

// Copyright ProjectR. All Rights Reserved.

#include "PRGameplayAbility_FaerinCrashSequence.h"

#include "DrawDebugHelpers.h"
#include "Engine/EngineTypes.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "ProjectR/AbilitySystem/Abilities/Boss/PRBossAbilityTagUtils.h"
#include "ProjectR/PRGameplayTags.h"

UPRGameplayAbility_FaerinCrashSequence::UPRGameplayAbility_FaerinCrashSequence()
{
	SetAssetTags(PRBossAbility::MakePatternAssetTags(PRGameplayTags::Ability_Boss_Faerin_CrashSequence));
	bRequireCharacterEventRouter = true;
}

void UPRGameplayAbility_FaerinCrashSequence::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	DamagedActorsThisCrash.Reset();
	bCrashDamageApplied = false;

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UPRGameplayAbility_FaerinCrashSequence::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const bool bReplicateEndAbility,
	const bool bWasCancelled)
{
	DamagedActorsThisCrash.Reset();
	bCrashDamageApplied = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UPRGameplayAbility_FaerinCrashSequence::NativeOnStageCharacterEvent(
	const FPRFaerinStagedMontageStage& Stage,
	const FName EventName)
{
	(void)Stage;

	if (EventName != CrashAreaEventName)
	{
		return;
	}

	if (bApplyCrashDamageOncePerActivation && bCrashDamageApplied)
	{
		return;
	}

	ApplyCrashAreaDamage();
	bCrashDamageApplied = true;
}

FVector UPRGameplayAbility_FaerinCrashSequence::ResolveCrashAreaCenter() const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor))
	{
		return FVector::ZeroVector;
	}

	return AvatarActor->GetActorTransform().TransformPositionNoScale(CrashAreaLocalOffset);
}

void UPRGameplayAbility_FaerinCrashSequence::ApplyCrashAreaDamage()
{
	if (CrashDamageMode == EPRFaerinCrashDamageMode::GlobalPlayers)
	{
		ApplyCrashGlobalPlayerDamage();
		return;
	}

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	UWorld* World = GetWorld();
	if (!IsValid(AvatarActor) || !IsValid(World) || CrashDamageRadius <= 0.0f)
	{
		return;
	}

	const FVector CrashCenter = ResolveCrashAreaCenter();
	const FCollisionShape CrashShape = FCollisionShape::MakeSphere(CrashDamageRadius);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(FaerinCrashAreaDamage), false, AvatarActor);
	QueryParams.AddIgnoredActor(AvatarActor);
	QueryParams.bFindInitialOverlaps = true;

	TArray<FOverlapResult> OverlapResults;
	World->OverlapMultiByChannel(
		OverlapResults,
		CrashCenter,
		FQuat::Identity,
		CrashOverlapChannel,
		CrashShape,
		QueryParams);

	if (bDrawDebugCrashArea)
	{
		DrawDebugSphere(World, CrashCenter, CrashDamageRadius, 32, FColor::Red, false, CrashDebugDrawDuration);
	}

	for (const FOverlapResult& OverlapResult : OverlapResults)
	{
		AActor* HitActor = OverlapResult.GetActor();
		const TWeakObjectPtr<AActor> HitActorKey(HitActor);
		if (!IsValid(HitActor) || HitActor == AvatarActor || DamagedActorsThisCrash.Contains(HitActorKey))
		{
			continue;
		}

		ApplyAttackPowerDamage(HitActor, CrashDamageMultiplier, CrashPoiseDamage);
		DamagedActorsThisCrash.Add(HitActorKey);
	}
}

void UPRGameplayAbility_FaerinCrashSequence::ApplyCrashGlobalPlayerDamage()
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	UWorld* World = GetWorld();
	if (!IsValid(AvatarActor) || !IsValid(World))
	{
		return;
	}

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PlayerController = It->Get();
		APawn* TargetPawn = IsValid(PlayerController) ? PlayerController->GetPawn() : nullptr;
		const TWeakObjectPtr<AActor> TargetKey(TargetPawn);
		if (!IsValid(TargetPawn)
			|| TargetPawn == AvatarActor
			|| DamagedActorsThisCrash.Contains(TargetKey))
		{
			continue;
		}

		ApplyAttackPowerDamage(TargetPawn, CrashDamageMultiplier, CrashPoiseDamage);
		DamagedActorsThisCrash.Add(TargetKey);
	}
}

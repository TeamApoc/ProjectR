// Copyright ProjectR. All Rights Reserved.

#include "PRGameplayAbility_FaerinNearTeleportApproach.h"

#include "AIController.h"
#include "Engine/World.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "GameFramework/Character.h"
#include "NavigationSystem.h"
#include "NiagaraSystem.h"
#include "ProjectR/AbilitySystem/Abilities/Boss/PRBossAbilityTagUtils.h"
#include "ProjectR/AI/PREnemyEQSSelectionUtils.h"
#include "ProjectR/Character/Enemy/Boss/PRFaerinCharacter.h"
#include "ProjectR/Character/Enemy/PREnemyBaseCharacter.h"
#include "ProjectR/PRGameplayTags.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogPRFaerinNearTeleport, Log, All);

namespace
{
	const FName NearTeleportGridSizeParamName(TEXT("NearTeleport_GridSize"));
	const FName NearTeleportSelfMaxDistanceParamName(TEXT("NearTeleport_SelfMaxDistance"));

	void UpsertFloatParam(TArray<FPREnemyEQSFloatParam>& InOutFloatParams, const FName ParamName, const float Value)
	{
		if (ParamName == NAME_None)
		{
			return;
		}

		for (FPREnemyEQSFloatParam& FloatParam : InOutFloatParams)
		{
			if (FloatParam.ParamName == ParamName)
			{
				FloatParam.Value = Value;
				return;
			}
		}

		FPREnemyEQSFloatParam NewFloatParam;
		NewFloatParam.ParamName = ParamName;
		NewFloatParam.Value = Value;
		InOutFloatParams.Add(NewFloatParam);
	}
}

UPRGameplayAbility_FaerinNearTeleportApproach::UPRGameplayAbility_FaerinNearTeleportApproach()
{
	SetAssetTags(PRBossAbility::MakePatternAssetTags(PRGameplayTags::Ability_Boss_Faerin_TeleportDash));

	ActivationBlockedTags.AddTag(PRGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(PRGameplayTags::State_Groggy);
	ActivationBlockedTags.AddTag(PRGameplayTags::State_PhaseTransitioning);
	ActivationBlockedTags.AddTag(PRGameplayTags::State_Boss_PatternPlaying);

	ActivationOwnedTags.AddTag(PRGameplayTags::State_Boss_PatternPlaying);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Boss_Pattern);
}

void UPRGameplayAbility_FaerinNearTeleportApproach::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	APREnemyBaseCharacter* EnemyCharacter = GetEnemyAvatarCharacter();
	ACharacter* BossCharacter = Cast<ACharacter>(EnemyCharacter);
	if (!IsValid(EnemyCharacter)
		|| !EnemyCharacter->HasAuthority()
		|| !IsValid(BossCharacter)
		|| !ResolveTeleportRequest(EnemyCharacter))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	bNearTeleportFinished = false;
	bOriginalActorCollisionEnabled = BossCharacter->GetActorEnableCollision();
	DisappearLocation = BossCharacter->GetActorLocation();
	bHasCachedReappearTransform = ResolveReappearTransform(CachedReappearLocation, CachedReappearRotation);
	if (!bHasCachedReappearTransform)
	{
		UE_LOG(LogPRFaerinNearTeleport, Warning, TEXT("[NearTeleport] 재등장 위치 계산 실패. Boss=%s Query=%s MaxDistance=%.1f"),
			*GetNameSafe(BossCharacter),
			*GetNameSafe(QueryTemplate.Get()),
			MaxDistanceFromSelf);
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UE_LOG(LogPRFaerinNearTeleport, Verbose, TEXT("[NearTeleport] 재등장 위치 확정. Boss=%s From=%s To=%s Dist2D=%.1f"),
		*GetNameSafe(BossCharacter),
		*DisappearLocation.ToCompactString(),
		*CachedReappearLocation.ToCompactString(),
		FVector::Dist2D(DisappearLocation, CachedReappearLocation));

	BeginDisappear(BossCharacter);
}

void UPRGameplayAbility_FaerinNearTeleportApproach::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ReappearTimerHandle);
	}

	if (ACharacter* BossCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		RestoreBossPresentation(BossCharacter);
	}

	ActiveDirectorComponent = nullptr;
	ActiveRequest = FPRFaerinNearTeleportRequest();
	CachedReappearLocation = FVector::ZeroVector;
	CachedReappearRotation = FRotator::ZeroRotator;
	bHasCachedReappearTransform = false;
	bNearTeleportFinished = true;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UPRGameplayAbility_FaerinNearTeleportApproach::ResolveTeleportRequest(APREnemyBaseCharacter* EnemyCharacter)
{
	if (!IsValid(EnemyCharacter))
	{
		return false;
	}

	ActiveDirectorComponent = EnemyCharacter->FindComponentByClass<UPRFaerinCombatDirectorComponent>();
	if (!IsValid(ActiveDirectorComponent))
	{
		return false;
	}

	if (!ActiveDirectorComponent->GetActiveNearTeleportRequest(ActiveRequest))
	{
		return false;
	}

	if (!ActiveRequest.bIsValid || !IsValid(ActiveRequest.TargetActor))
	{
		return false;
	}

	if (ActiveRequest.PlacementMode == EPRFaerinNearTeleportPlacementMode::TargetBack)
	{
		return ActiveRequest.TargetBackDistance > 0.0f;
	}

	return IsValid(QueryTemplate.Get()) && MaxDistanceFromSelf > 0.0f;
}

void UPRGameplayAbility_FaerinNearTeleportApproach::BeginDisappear(ACharacter* BossCharacter)
{
	if (!IsValid(BossCharacter))
	{
		FinishNearTeleport(true);
		return;
	}

	if (AAIController* AIController = Cast<AAIController>(BossCharacter->GetController()))
	{
		AIController->StopMovement();
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
	}

	BossCharacter->SetActorEnableCollision(false);
	SpawnBodyNiagara(DisappearDissolveNiagaraSystem);
	SpawnBodyNiagara(TeleportInNiagaraSystem);
	BossCharacter->ForceNetUpdate();

	if (DisappearPresentationSeconds <= 0.0f)
	{
		CommitDisappearAndScheduleReappear();
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			ReappearTimerHandle,
			this,
			&UPRGameplayAbility_FaerinNearTeleportApproach::CommitDisappearAndScheduleReappear,
			DisappearPresentationSeconds,
			false);
	}
}

void UPRGameplayAbility_FaerinNearTeleportApproach::CommitDisappearAndScheduleReappear()
{
	if (bNearTeleportFinished)
	{
		return;
	}

	ACharacter* BossCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(BossCharacter))
	{
		FinishNearTeleport(true);
		return;
	}

	if (APRFaerinCharacter* FaerinCharacter = Cast<APRFaerinCharacter>(BossCharacter))
	{
		FaerinCharacter->Multicast_SetNearTeleportHidden(true);
	}
	else
	{
		BossCharacter->SetActorHiddenInGame(true);
	}
	BossCharacter->ForceNetUpdate();

	const float RemainingDelaySeconds = FMath::Max(ReappearDelaySeconds - DisappearPresentationSeconds, 0.0f);
	if (RemainingDelaySeconds <= 0.0f)
	{
		ReappearNearSelf();
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			ReappearTimerHandle,
			this,
			&UPRGameplayAbility_FaerinNearTeleportApproach::ReappearNearSelf,
			RemainingDelaySeconds,
			false);
	}
}

void UPRGameplayAbility_FaerinNearTeleportApproach::ReappearNearSelf()
{
	if (bNearTeleportFinished)
	{
		return;
	}

	ACharacter* BossCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(BossCharacter))
	{
		FinishNearTeleport(true);
		return;
	}

	if (!bHasCachedReappearTransform)
	{
		FinishNearTeleport(true);
		return;
	}

	BossCharacter->SetActorLocationAndRotation(
		CachedReappearLocation,
		CachedReappearRotation,
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	if (APRFaerinCharacter* FaerinCharacter = Cast<APRFaerinCharacter>(BossCharacter))
	{
		FaerinCharacter->Multicast_PlayNearTeleportReappearPresentation(
			CachedReappearLocation,
			CachedReappearRotation,
			TeleportOutNiagaraSystem,
			ReappearDissolveNiagaraSystem,
			BodyNiagaraAttachSocketName);
	}
	else
	{
		RestoreBossPresentation(BossCharacter);
		SpawnBodyNiagara(TeleportOutNiagaraSystem);
		SpawnBodyNiagara(ReappearDissolveNiagaraSystem);
	}
	BossCharacter->SetActorEnableCollision(bOriginalActorCollisionEnabled);
	BossCharacter->ForceNetUpdate();

	FinishNearTeleport(false);
}

bool UPRGameplayAbility_FaerinNearTeleportApproach::ResolveReappearTransform(
	FVector& OutLocation,
	FRotator& OutRotation) const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	const AActor* TargetActor = ActiveRequest.TargetActor.Get();
	if (!IsValid(AvatarActor))
	{
		return false;
	}

	const bool bUseTargetBackPlacement = ActiveRequest.PlacementMode == EPRFaerinNearTeleportPlacementMode::TargetBack;
	bool bUsedHomeFallback = false;
	if (bUseTargetBackPlacement)
	{
		if (!ResolveTargetBackReappearLocation(OutLocation))
		{
			return false;
		}
	}
	else if (!ResolveEQSReappearLocation(OutLocation))
	{
		if (!ResolveHomeReappearLocation(OutLocation))
		{
			return false;
		}

		bUsedHomeFallback = true;
		UE_LOG(LogPRFaerinNearTeleport, Warning, TEXT("[NearTeleport] EQS 실패로 HomeLocation을 재등장 위치로 사용한다. Boss=%s Home=%s"),
			*GetNameSafe(AvatarActor),
			*OutLocation.ToCompactString());
	}

	if (!bUseTargetBackPlacement)
	{
		if (UWorld* World = GetWorld())
		{
			if (UNavigationSystemV1* NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
			{
				FNavLocation ProjectedLocation;
				if (NavigationSystem->ProjectPointToNavigation(OutLocation, ProjectedLocation, ReappearNavProjectExtent))
				{
					OutLocation = ProjectedLocation.Location;
				}
			}
		}
	}

	if (!bUseTargetBackPlacement
		&& !bUsedHomeFallback
		&& FVector::Dist2D(DisappearLocation, OutLocation) > MaxDistanceFromSelf)
	{
		return false;
	}

	if (IsValid(TargetActor))
	{
		FVector FacingDirection = TargetActor->GetActorLocation() - OutLocation;
		FacingDirection.Z = 0.0f;
		if (FacingDirection.Normalize())
		{
			OutRotation = FacingDirection.Rotation();
			return true;
		}
	}

	OutRotation = AvatarActor->GetActorRotation();
	return true;
}

bool UPRGameplayAbility_FaerinNearTeleportApproach::ResolveTargetBackReappearLocation(FVector& OutLocation) const
{
	const AActor* TargetActor = ActiveRequest.TargetActor.Get();
	if (!IsValid(TargetActor) || ActiveRequest.TargetBackDistance <= 0.0f)
	{
		return false;
	}

	if (ResolveTargetRelativeReappearLocation(*TargetActor, -1.0f, OutLocation))
	{
		return true;
	}

	return ResolveTargetRelativeReappearLocation(*TargetActor, 1.0f, OutLocation);
}

bool UPRGameplayAbility_FaerinNearTeleportApproach::ResolveTargetRelativeReappearLocation(
	const AActor& TargetActor,
	const float DirectionSign,
	FVector& OutLocation) const
{
	FVector TargetForward = TargetActor.GetActorForwardVector();
	TargetForward.Z = 0.0f;
	if (!TargetForward.Normalize())
	{
		return false;
	}

	const FVector CandidateDirection = TargetForward * FMath::Sign(DirectionSign);
	const FVector RawLocation = TargetActor.GetActorLocation() + CandidateDirection * ActiveRequest.TargetBackDistance;
	UWorld* World = GetWorld();
	UNavigationSystemV1* NavigationSystem = World != nullptr
		? FNavigationSystem::GetCurrent<UNavigationSystemV1>(World)
		: nullptr;
	if (!IsValid(NavigationSystem))
	{
		return false;
	}

	FNavLocation ProjectedLocation;
	if (!NavigationSystem->ProjectPointToNavigation(
			RawLocation,
			ProjectedLocation,
			ActiveRequest.TargetBackNavProjectExtent))
	{
		return false;
	}

	FVector ProjectedDirection = ProjectedLocation.Location - TargetActor.GetActorLocation();
	ProjectedDirection.Z = 0.0f;
	if (!ProjectedDirection.Normalize())
	{
		return false;
	}

	if (FVector::DotProduct(ProjectedDirection, CandidateDirection) <= 0.0f)
	{
		return false;
	}

	OutLocation = ProjectedLocation.Location;
	return true;
}

bool UPRGameplayAbility_FaerinNearTeleportApproach::ResolveEQSReappearLocation(FVector& OutLocation) const
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor) || !IsValid(QueryTemplate.Get()))
	{
		return false;
	}

	TArray<FPREnemyEQSFloatParam> RuntimeFloatParams = FloatParams;
	UpsertFloatParam(RuntimeFloatParams, NearTeleportGridSizeParamName, MaxDistanceFromSelf);
	UpsertFloatParam(RuntimeFloatParams, NearTeleportSelfMaxDistanceParamName, MaxDistanceFromSelf);

	if (!PREnemyEQSSelectionUtils::RunLocationQuery(
			AvatarActor,
			QueryTemplate.Get(),
			RuntimeFloatParams,
			QueryRunMode,
			CandidateSelectionMode,
			TopCandidateCount,
			TopScoreCandidateRatio,
			OutLocation))
	{
		return false;
	}

	if (FVector::Dist2D(DisappearLocation, OutLocation) > MaxDistanceFromSelf)
	{
		return false;
	}

	return true;
}

bool UPRGameplayAbility_FaerinNearTeleportApproach::ResolveHomeReappearLocation(FVector& OutLocation) const
{
	const APREnemyBaseCharacter* EnemyCharacter = Cast<APREnemyBaseCharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(EnemyCharacter))
	{
		return false;
	}

	OutLocation = EnemyCharacter->GetHomeLocation();
	return !OutLocation.ContainsNaN();
}

void UPRGameplayAbility_FaerinNearTeleportApproach::SpawnBodyNiagara(UNiagaraSystem* NiagaraSystem) const
{
	if (!IsValid(NiagaraSystem))
	{
		return;
	}

	APRFaerinCharacter* FaerinCharacter = Cast<APRFaerinCharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(FaerinCharacter))
	{
		return;
	}

	FaerinCharacter->Multicast_SpawnNearTeleportBodyNiagara(NiagaraSystem, BodyNiagaraAttachSocketName);
}

void UPRGameplayAbility_FaerinNearTeleportApproach::RestoreBossPresentation(ACharacter* BossCharacter) const
{
	if (!IsValid(BossCharacter))
	{
		return;
	}

	if (APRFaerinCharacter* FaerinCharacter = Cast<APRFaerinCharacter>(BossCharacter))
	{
		FaerinCharacter->Multicast_SetNearTeleportHidden(false);
	}
	else
	{
		BossCharacter->SetActorHiddenInGame(false);
	}
	BossCharacter->SetActorEnableCollision(bOriginalActorCollisionEnabled);
}

void UPRGameplayAbility_FaerinNearTeleportApproach::FinishNearTeleport(bool bWasCancelled)
{
	if (bNearTeleportFinished)
	{
		return;
	}

	bNearTeleportFinished = true;
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bWasCancelled);
}

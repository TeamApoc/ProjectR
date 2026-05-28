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
		|| !ResolveTeleportRequest(EnemyCharacter)
		|| !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	bNearTeleportFinished = false;
	bOriginalActorCollisionEnabled = BossCharacter->GetActorEnableCollision();
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

	return ActiveRequest.bIsValid
		&& IsValid(ActiveRequest.TargetActor)
		&& IsValid(ActiveRequest.QueryTemplate.Get())
		&& ActiveRequest.MaxDistanceFromSelf > 0.0f;
}

void UPRGameplayAbility_FaerinNearTeleportApproach::BeginDisappear(ACharacter* BossCharacter)
{
	if (!IsValid(BossCharacter))
	{
		FinishNearTeleport(true);
		return;
	}

	DisappearLocation = BossCharacter->GetActorLocation();
	SpawnBodyNiagara(DisappearDissolveNiagaraSystem);
	SpawnBodyNiagara(TeleportInNiagaraSystem);

	if (AAIController* AIController = Cast<AAIController>(BossCharacter->GetController()))
	{
		AIController->StopMovement();
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
	}

	if (APRFaerinCharacter* FaerinCharacter = Cast<APRFaerinCharacter>(BossCharacter))
	{
		FaerinCharacter->Multicast_SetNearTeleportHidden(true);
	}
	else
	{
		BossCharacter->SetActorHiddenInGame(true);
	}
	BossCharacter->SetActorEnableCollision(false);
	BossCharacter->ForceNetUpdate();

	if (ActiveRequest.ReappearDelaySeconds <= 0.0f)
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
			ActiveRequest.ReappearDelaySeconds,
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

	FVector ReappearLocation = FVector::ZeroVector;
	FRotator ReappearRotation = FRotator::ZeroRotator;
	if (!ResolveReappearTransform(ReappearLocation, ReappearRotation))
	{
		FinishNearTeleport(true);
		return;
	}

	BossCharacter->SetActorLocationAndRotation(
		ReappearLocation,
		ReappearRotation,
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	RestoreBossPresentation(BossCharacter);
	SpawnBodyNiagara(TeleportOutNiagaraSystem);
	SpawnBodyNiagara(ReappearDissolveNiagaraSystem);
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

	if (!ResolveEQSReappearLocation(OutLocation))
	{
		return false;
	}

	if (UWorld* World = GetWorld())
	{
		if (UNavigationSystemV1* NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
		{
			FNavLocation ProjectedLocation;
			if (NavigationSystem->ProjectPointToNavigation(OutLocation, ProjectedLocation, ActiveRequest.NavProjectExtent))
			{
				OutLocation = ProjectedLocation.Location;
			}
		}
	}

	if (FVector::Dist2D(DisappearLocation, OutLocation) > ActiveRequest.MaxDistanceFromSelf)
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

bool UPRGameplayAbility_FaerinNearTeleportApproach::ResolveEQSReappearLocation(FVector& OutLocation) const
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor) || !IsValid(ActiveRequest.QueryTemplate.Get()))
	{
		return false;
	}

	if (!PREnemyEQSSelectionUtils::RunLocationQuery(
			AvatarActor,
			ActiveRequest.QueryTemplate.Get(),
			ActiveRequest.FloatParams,
			ActiveRequest.QueryRunMode,
			ActiveRequest.CandidateSelectionMode,
			ActiveRequest.TopCandidateCount,
			ActiveRequest.TopScoreCandidateRatio,
			OutLocation))
	{
		return false;
	}

	if (FVector::Dist2D(DisappearLocation, OutLocation) > ActiveRequest.MaxDistanceFromSelf)
	{
		return false;
	}

	return true;
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

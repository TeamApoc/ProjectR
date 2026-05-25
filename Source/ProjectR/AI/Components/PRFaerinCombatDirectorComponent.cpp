// Copyright ProjectR. All Rights Reserved.

#include "PRFaerinCombatDirectorComponent.h"

#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Engine/World.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Common.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/AI/Data/PRPatternDataAsset.h"
#include "ProjectR/AI/PREnemyAIDebug.h"
#include "ProjectR/AI/PREnemyPatternSelectionUtils.h"
#include "ProjectR/Character/Enemy/PRBossBaseCharacter.h"
#include "ProjectR/Character/Enemy/PREnemyBaseCharacter.h"
#include "ProjectR/PRGameplayTags.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogPRFaerinCombatDirector, Log, All);

namespace
{
	struct FPRFaerinPatternCandidate
	{
		const FPRPatternRule* PatternRule = nullptr;
		const FPRFaerinPatternLoopMetadata* LoopMetadata = nullptr;
		float FinalWeight = 0.0f;
	};
}

// ===== 초기화 =====

UPRFaerinCombatDirectorComponent::UPRFaerinCombatDirectorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}

void UPRFaerinCombatDirectorComponent::BeginPlay()
{
	Super::BeginPlay();

	InitializeDirector();

	if (bValidateOnBeginPlay && IsValid(LoopDataAsset))
	{
		TArray<FString> ValidationErrors;
		LoopDataAsset->ValidateLoopData(PatternDataAsset, nullptr, ValidationErrors);
		LogValidationErrors(ValidationErrors);
	}
}

void UPRFaerinCombatDirectorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CancelCurrentLoopStep();
	Super::EndPlay(EndPlayReason);
}

// ===== 루프 실행 =====

bool UPRFaerinCombatDirectorComponent::RunNextLoopStep(UBehaviorTreeComponent& OwnerComp)
{
	if (!InitializeDirector())
	{
		if (!bHasLoggedInitializationError)
		{
			UE_LOG(
				LogPRFaerinCombatDirector,
				Warning,
				TEXT("Faerin CombatDirector 초기화 실패. Owner=%s, LoopDataAsset=%s, PatternDataAsset=%s, ASC=%s"),
				*GetNameSafe(GetOwner()),
				*GetNameSafe(LoopDataAsset),
				*GetNameSafe(PatternDataAsset),
				*GetNameSafe(AbilitySystemComponent));
			bHasLoggedInitializationError = true;
		}
		return false;
	}

	bHasLoggedInitializationError = false;

	if (!CanRunLoopStep())
	{
		return false;
	}

	if (!OwnerEnemy->HasAuthority())
	{
		return false;
	}

	if (AbilitySystemComponent->HasMatchingGameplayTag(PRGameplayTags::State_Dead)
		|| AbilitySystemComponent->HasMatchingGameplayTag(PRGameplayTags::State_Groggy)
		|| AbilitySystemComponent->HasMatchingGameplayTag(PRGameplayTags::State_PhaseTransitioning))
	{
		return false;
	}

	if (!bHasLoggedRuntimeValidationErrors && IsValid(LoopDataAsset))
	{
		TArray<FString> ValidationErrors;
		LoopDataAsset->ValidateLoopData(PatternDataAsset, AbilitySystemComponent, ValidationErrors);
		LogValidationErrors(ValidationErrors);
		bHasLoggedRuntimeValidationErrors = true;
	}

	LoopState = EPRFaerinCombatLoopState::SelectingPattern;

	FPRFaerinPatternPlan SelectedPlan;
	if (!SelectPatternPlan(OwnerComp, SelectedPlan))
	{
		if (StartOutOfRangeApproach(OwnerComp))
		{
			return true;
		}

		LoopState = EPRFaerinCombatLoopState::Idle;
		return false;
	}

	ActivePatternPlan = SelectedPlan;
	if (!ActivatePatternAbility(ActivePatternPlan))
	{
		LoopState = EPRFaerinCombatLoopState::Idle;
		ActivePatternPlan = FPRFaerinPatternPlan();
		ActiveTarget = nullptr;
		return false;
	}

	return true;
}

void UPRFaerinCombatDirectorComponent::CancelCurrentLoopStep()
{
	if (LoopState == EPRFaerinCombatLoopState::Idle)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LoopStepFailSafeTimerHandle);
		World->GetTimerManager().ClearTimer(PostPatternStrafeTimerHandle);
	}

	if (IsObservedAbilityActive())
	{
		AbilitySystemComponent->CancelAbilityHandle(ActiveAbilityHandle);
	}

	if (AAIController* AIController = GetOwnerAIController())
	{
		AIController->StopMovement();
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
	}

	if (IsValid(OwnerEnemy))
	{
		OwnerEnemy->ClearCombatMovePresentationContext();
	}

	ClearAbilityEndDelegate();
	ActivePatternPlan = FPRFaerinPatternPlan();
	ClearActiveApproachSprintRequest();
	ActiveAbilityHandle = FGameplayAbilitySpecHandle();
	ObservedAbilityRole = EPRFaerinObservedAbilityRole::None;
	ActiveTarget = nullptr;
	LoopState = EPRFaerinCombatLoopState::Idle;
}

bool UPRFaerinCombatDirectorComponent::CanRunLoopStep() const
{
	return LoopState == EPRFaerinCombatLoopState::Idle
		&& IsValid(LoopDataAsset);
}

float UPRFaerinCombatDirectorComponent::CalculateStrafeDuration() const
{
	if (!IsValid(LoopDataAsset))
	{
		return 0.0f;
	}

	const FPRFaerinPhaseLoopConfig* PhaseConfig = LoopDataAsset->FindPhaseConfig(ResolveCurrentPhase());
	if (PhaseConfig == nullptr)
	{
		return 0.0f;
	}

	const float HealthRatio = ResolveHealthRatio();
	const float RatioRange = PhaseConfig->HighHealthRatioReference - PhaseConfig->LowHealthRatioReference;
	if (RatioRange <= UE_SMALL_NUMBER)
	{
		return PhaseConfig->StrafeDurationAtHighHealth;
	}

	return FMath::GetMappedRangeValueClamped(
		FVector2D(PhaseConfig->LowHealthRatioReference, PhaseConfig->HighHealthRatioReference),
		FVector2D(PhaseConfig->StrafeDurationAtLowHealth, PhaseConfig->StrafeDurationAtHighHealth),
		HealthRatio);
}

bool UPRFaerinCombatDirectorComponent::GetActiveApproachSprintRequest(FPRFaerinApproachSprintRequest& OutRequest) const
{
	OutRequest = ActiveApproachSprintRequest;
	return OutRequest.bIsValid && IsValid(OutRequest.TargetActor);
}

bool UPRFaerinCombatDirectorComponent::InitializeDirector()
{
	OwnerEnemy = Cast<APREnemyBaseCharacter>(GetOwner());
	if (!IsValid(OwnerEnemy))
	{
		return false;
	}

	AbilitySystemComponent = OwnerEnemy->GetEnemyAbilitySystemComponent();
	PatternDataAsset = OwnerEnemy->GetPatternDataAsset();

	return IsValid(AbilitySystemComponent)
		&& IsValid(PatternDataAsset)
		&& IsValid(LoopDataAsset);
}

bool UPRFaerinCombatDirectorComponent::SelectPatternPlan(UBehaviorTreeComponent& OwnerComp, FPRFaerinPatternPlan& OutPlan)
{
	OutPlan = FPRFaerinPatternPlan();

	FPREnemyPatternQueryRuntime PatternRuntime;
	if (!PREnemyPatternSelectionUtils::BuildPatternQueryRuntime(
		OwnerComp,
		CurrentTargetKey,
		HasLOSKey,
		ChargePathClearKey,
		TacticalModeKey,
		AttackPressureKey,
		PatternRuntime))
	{
		return false;
	}

	TArray<const FPRPatternRule*> MatchedRules;
	PREnemyPatternSelectionUtils::CollectMatchingPatternRules(
		PatternRuntime,
		PatternCategoryFilter,
		PatternGroupFilter,
		true,
		EPRPatternContextMatchMode::FullMatch,
		MatchedRules);

	TArray<FPRFaerinPatternCandidate> Candidates;
	float TotalWeight = 0.0f;
	for (const FPRPatternRule* PatternRule : MatchedRules)
	{
		if (PatternRule == nullptr)
		{
			continue;
		}

		const FPRFaerinPatternLoopMetadata* LoopMetadata = LoopDataAsset->FindPatternMetadata(PatternRule->AbilityTag);
		if (LoopMetadata == nullptr || !LoopMetadata->bEnabled || LoopMetadata->SelectionWeightScale <= 0.0f)
		{
			continue;
		}

		FPRFaerinPatternCandidate Candidate;
		Candidate.PatternRule = PatternRule;
		Candidate.LoopMetadata = LoopMetadata;
		Candidate.FinalWeight = PatternRule->SelectionWeight * LoopMetadata->SelectionWeightScale;
		Candidates.Add(Candidate);
		TotalWeight += Candidate.FinalWeight;
	}

	if (Candidates.IsEmpty() || TotalWeight <= 0.0f)
	{
		return false;
	}

	const float PickValue = FMath::FRandRange(0.0f, TotalWeight);
	float AccumulatedWeight = 0.0f;
	const FPRFaerinPatternCandidate* SelectedCandidate = &Candidates.Last();
	for (const FPRFaerinPatternCandidate& Candidate : Candidates)
	{
		AccumulatedWeight += Candidate.FinalWeight;
		if (PickValue <= AccumulatedWeight)
		{
			SelectedCandidate = &Candidate;
			break;
		}
	}

	OutPlan.AbilityTag = SelectedCandidate->PatternRule->AbilityTag;
	OutPlan.PatternRule = *SelectedCandidate->PatternRule;
	OutPlan.LoopMetadata = *SelectedCandidate->LoopMetadata;
	OutPlan.FinalSelectionWeight = SelectedCandidate->FinalWeight;
	ActiveTarget = PatternRuntime.CurrentTarget;

	LogSelectedPattern(OutPlan, PatternRuntime);
	return true;
}

bool UPRFaerinCombatDirectorComponent::StartOutOfRangeApproach(UBehaviorTreeComponent& OwnerComp)
{
	if (IsApproachSprintRepeatBlocked())
	{
		return false;
	}

	if (!IsValid(LoopDataAsset))
	{
		return false;
	}

	const FPRFaerinPhaseLoopConfig* PhaseConfig = LoopDataAsset->FindPhaseConfig(ResolveCurrentPhase());
	if (PhaseConfig == nullptr || !PhaseConfig->bUsePostStrafeApproach)
	{
		return false;
	}

	FPRFaerinPatternPlan ApproachPlan;
	if (!SelectOutOfRangeApproachPlan(OwnerComp, *PhaseConfig, ApproachPlan))
	{
		return false;
	}

	ActivePatternPlan = ApproachPlan;
	if (StartPostStrafeApproach(*PhaseConfig))
	{
		return true;
	}

	ActivePatternPlan = FPRFaerinPatternPlan();
	ActiveTarget = nullptr;
	LoopState = EPRFaerinCombatLoopState::SelectingPattern;
	return false;
}

bool UPRFaerinCombatDirectorComponent::SelectOutOfRangeApproachPlan(
	UBehaviorTreeComponent& OwnerComp,
	const FPRFaerinPhaseLoopConfig& PhaseConfig,
	FPRFaerinPatternPlan& OutPlan)
{
	OutPlan = FPRFaerinPatternPlan();

	if (!PhaseConfig.ApproachAbilityTag.IsValid())
	{
		return false;
	}

	FPREnemyPatternQueryRuntime PatternRuntime;
	if (!PREnemyPatternSelectionUtils::BuildPatternQueryRuntime(
		OwnerComp,
		CurrentTargetKey,
		HasLOSKey,
		ChargePathClearKey,
		TacticalModeKey,
		AttackPressureKey,
		PatternRuntime))
	{
		return false;
	}

	TArray<const FPRPatternRule*> RangeIgnoredRules;
	PREnemyPatternSelectionUtils::CollectMatchingPatternRules(
		PatternRuntime,
		PatternCategoryFilter,
		PatternGroupFilter,
		true,
		EPRPatternContextMatchMode::IgnoreRange,
		RangeIgnoredRules);

	const FPRPatternRule* BestRule = nullptr;
	const FPRFaerinPatternLoopMetadata* BestMetadata = nullptr;
	float BestWeight = 0.0f;
	for (const FPRPatternRule* PatternRule : RangeIgnoredRules)
	{
		if (PatternRule == nullptr || PatternRuntime.PatternContext.DistanceToTarget <= PatternRule->MaxRange)
		{
			continue;
		}

		const FPRFaerinPatternLoopMetadata* LoopMetadata = LoopDataAsset->FindPatternMetadata(PatternRule->AbilityTag);
		if (LoopMetadata == nullptr || !LoopMetadata->bEnabled || LoopMetadata->SelectionWeightScale <= 0.0f)
		{
			continue;
		}

		const float CandidateWeight = PatternRule->SelectionWeight * LoopMetadata->SelectionWeightScale;
		const bool bPreferCandidate = BestRule == nullptr
			|| CandidateWeight > BestWeight
			|| (FMath::IsNearlyEqual(CandidateWeight, BestWeight) && PatternRule->MaxRange > BestRule->MaxRange);
		if (bPreferCandidate)
		{
			BestRule = PatternRule;
			BestMetadata = LoopMetadata;
			BestWeight = CandidateWeight;
		}
	}

	if (BestRule == nullptr || BestMetadata == nullptr)
	{
		return false;
	}

	OutPlan.AbilityTag = BestRule->AbilityTag;
	OutPlan.PatternRule = *BestRule;
	OutPlan.LoopMetadata = *BestMetadata;
	OutPlan.FinalSelectionWeight = BestWeight;
	ActiveTarget = PatternRuntime.CurrentTarget;

	if (PREnemyAIDebug::IsPatternLogEnabled())
	{
		UE_LOG(
			LogPRFaerinCombatDirector,
			Verbose,
			TEXT("Faerin 사거리 밖 sprint 접근 선택. Owner=%s, AbilityTag=%s, Distance=%.1f, MaxRange=%.1f"),
			*GetNameSafe(PatternRuntime.ControlledPawn),
			*OutPlan.AbilityTag.ToString(),
			PatternRuntime.PatternContext.DistanceToTarget,
			OutPlan.PatternRule.MaxRange);
	}

	return true;
}

bool UPRFaerinCombatDirectorComponent::ActivatePatternAbility(const FPRFaerinPatternPlan& PatternPlan)
{
	if (!IsValid(AbilitySystemComponent) || !PatternPlan.AbilityTag.IsValid())
	{
		return false;
	}

	if (!AbilitySystemComponent->TryActivateAbilityOnServer(PatternPlan.AbilityTag, ActiveAbilityHandle))
	{
		UE_LOG(
			LogPRFaerinCombatDirector,
			Warning,
			TEXT("Faerin 패턴 Ability 활성화에 실패했다. Owner=%s, AbilityTag=%s"),
			*GetNameSafe(GetOwner()),
			*PatternPlan.AbilityTag.ToString());
		return false;
	}

	LoopState = EPRFaerinCombatLoopState::WaitingPatternEnd;
	ObservedAbilityRole = EPRFaerinObservedAbilityRole::Pattern;
	BindAbilityEndDelegate();

	if (LoopStepFailSafeSeconds > 0.0f)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				LoopStepFailSafeTimerHandle,
				this,
				&UPRFaerinCombatDirectorComponent::HandleLoopStepFailSafeElapsed,
				LoopStepFailSafeSeconds,
				false);
		}
	}

	if (IsObservedAbilityActive())
	{
		return true;
	}

	QueueFinishLoopStep(true);
	return true;
}

void UPRFaerinCombatDirectorComponent::BindAbilityEndDelegate()
{
	ClearAbilityEndDelegate();

	UAbilitySystemComponent* BaseAbilitySystemComponent = AbilitySystemComponent.Get();
	if (!IsValid(BaseAbilitySystemComponent))
	{
		return;
	}

	AbilityEndedDelegateHandle = BaseAbilitySystemComponent->OnAbilityEnded.AddUObject(
		this,
		&UPRFaerinCombatDirectorComponent::HandleObservedAbilityEnded);
}

void UPRFaerinCombatDirectorComponent::ClearAbilityEndDelegate()
{
	if (!AbilityEndedDelegateHandle.IsValid())
	{
		return;
	}

	UAbilitySystemComponent* BaseAbilitySystemComponent = AbilitySystemComponent.Get();
	if (IsValid(BaseAbilitySystemComponent))
	{
		BaseAbilitySystemComponent->OnAbilityEnded.Remove(AbilityEndedDelegateHandle);
	}

	AbilityEndedDelegateHandle = FDelegateHandle();
}

bool UPRFaerinCombatDirectorComponent::IsObservedAbilityActive() const
{
	if (!IsValid(AbilitySystemComponent) || !ActiveAbilityHandle.IsValid())
	{
		return false;
	}

	const FGameplayAbilitySpec* ActiveSpec = AbilitySystemComponent->FindAbilitySpecFromHandle(ActiveAbilityHandle);
	return ActiveSpec != nullptr && ActiveSpec->IsActive();
}

void UPRFaerinCombatDirectorComponent::HandleObservedAbilityEnded(const FAbilityEndedData& EndedData)
{
	if (!ActiveAbilityHandle.IsValid() || EndedData.AbilitySpecHandle != ActiveAbilityHandle)
	{
		return;
	}

	const EPRFaerinObservedAbilityRole FinishedRole = ObservedAbilityRole;
	const bool bSucceeded = !EndedData.bWasCancelled;
	ClearAbilityEndDelegate();
	ActiveAbilityHandle = FGameplayAbilitySpecHandle();
	ObservedAbilityRole = EPRFaerinObservedAbilityRole::None;

	if (FinishedRole == EPRFaerinObservedAbilityRole::Pattern)
	{
		HandlePatternExecutionFinished(bSucceeded);
		return;
	}

	if (FinishedRole == EPRFaerinObservedAbilityRole::Approach)
	{
		if (UWorld* World = GetWorld())
		{
			LastApproachSprintEndTime = World->GetTimeSeconds();
		}

		ClearActiveApproachSprintRequest();
		FinishLoopStep(bSucceeded);
		return;
	}

	FinishLoopStep(bSucceeded);
}

void UPRFaerinCombatDirectorComponent::HandlePatternExecutionFinished(bool bSucceeded)
{
	if (!bSucceeded)
	{
		FinishLoopStep(false);
		return;
	}

	if (!IsValid(LoopDataAsset))
	{
		FinishLoopStep(true);
		return;
	}

	const FPRFaerinPhaseLoopConfig* PhaseConfig = LoopDataAsset->FindPhaseConfig(ResolveCurrentPhase());
	if (PhaseConfig != nullptr
		&& ShouldRunUrgentPatternRangeApproach(ActivePatternPlan, *PhaseConfig)
		&& StartPostStrafeApproach(*PhaseConfig))
	{
		return;
	}

	if (PhaseConfig != nullptr
		&& ShouldRunPostPatternStrafe(ActivePatternPlan, *PhaseConfig)
		&& StartPostPatternStrafe(*PhaseConfig))
	{
		return;
	}

	if (PhaseConfig != nullptr && StartPostStrafeApproach(*PhaseConfig))
	{
		return;
	}

	FinishLoopStep(true);
}

bool UPRFaerinCombatDirectorComponent::ShouldRunPostPatternStrafe(
	const FPRFaerinPatternPlan& PatternPlan,
	const FPRFaerinPhaseLoopConfig& PhaseConfig) const
{
	if (PatternPlan.LoopMetadata.PostPatternPolicy == EPRFaerinPostPatternPolicy::ForceImmediateNext)
	{
		return false;
	}

	if (PatternPlan.LoopMetadata.PostPatternPolicy == EPRFaerinPostPatternPolicy::ForceStrafe)
	{
		return true;
	}

	return PhaseConfig.bUsePostPatternStrafe;
}

bool UPRFaerinCombatDirectorComponent::ShouldRunUrgentPatternRangeApproach(
	const FPRFaerinPatternPlan& PatternPlan,
	const FPRFaerinPhaseLoopConfig& PhaseConfig) const
{
	if (!PhaseConfig.bUsePostStrafeApproach || !IsValid(ActiveTarget))
	{
		return false;
	}

	const EPRFaerinApproachPolicy ApproachPolicy = ResolveApproachPolicy(PatternPlan, PhaseConfig);
	if (ApproachPolicy == EPRFaerinApproachPolicy::None
		|| ApproachPolicy == EPRFaerinApproachPolicy::ShiftClose)
	{
		return false;
	}

	const AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor))
	{
		return false;
	}

	const float DistanceToTarget = FVector::Dist2D(OwnerActor->GetActorLocation(), ActiveTarget->GetActorLocation());
	return DistanceToTarget > PatternPlan.PatternRule.MaxRange;
}

bool UPRFaerinCombatDirectorComponent::StartPostPatternStrafe(const FPRFaerinPhaseLoopConfig& PhaseConfig)
{
	const float StrafeDuration = CalculateStrafeDuration();
	if (StrafeDuration <= 0.0f || !IsValid(ActiveTarget))
	{
		return false;
	}

	FVector StrafeDestination = FVector::ZeroVector;
	if (!ResolvePostPatternStrafeDestination(PhaseConfig, StrafeDestination))
	{
		return false;
	}

	AAIController* AIController = GetOwnerAIController();
	if (!IsValid(AIController) || !IsValid(OwnerEnemy))
	{
		return false;
	}

	LoopState = EPRFaerinCombatLoopState::PostPatternStrafe;
	OwnerEnemy->ApplyCombatMovePresentationContext(PhaseConfig.StrafePresentationConfig);
	AIController->SetFocus(ActiveTarget, EAIFocusPriority::Gameplay);

	const EPathFollowingRequestResult::Type MoveResult = AIController->MoveToLocation(
		StrafeDestination,
		PhaseConfig.StrafeAcceptanceRadius,
		true,
		true,
		true,
		true,
		nullptr,
		true);

	if (MoveResult == EPathFollowingRequestResult::Failed)
	{
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
		OwnerEnemy->ClearCombatMovePresentationContext();
		return false;
	}

	if (PhaseConfig.bAlternateStrafeDirection)
	{
		NextStrafeDirectionSign *= -1;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			PostPatternStrafeTimerHandle,
			this,
			&UPRFaerinCombatDirectorComponent::HandlePostPatternStrafeElapsed,
			StrafeDuration,
			false);
		return true;
	}

	return false;
}

bool UPRFaerinCombatDirectorComponent::ResolvePostPatternStrafeDestination(
	const FPRFaerinPhaseLoopConfig& PhaseConfig,
	FVector& OutDestination) const
{
	const AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor) || !IsValid(ActiveTarget))
	{
		return false;
	}

	FVector FromTarget = OwnerActor->GetActorLocation() - ActiveTarget->GetActorLocation();
	FromTarget.Z = 0.0f;

	const float CurrentDistance = FromTarget.Size();
	if (!FromTarget.Normalize())
	{
		FromTarget = -ActiveTarget->GetActorForwardVector();
		FromTarget.Z = 0.0f;
		if (!FromTarget.Normalize())
		{
			FromTarget = FVector::ForwardVector;
		}
	}

	const float StrafeRadius = PhaseConfig.StrafeRadiusOverride > 0.0f
		? PhaseConfig.StrafeRadiusOverride
		: FMath::Max(CurrentDistance, 300.0f);
	const float SignedArcAngle = PhaseConfig.StrafeArcAngleDegrees * static_cast<float>(NextStrafeDirectionSign);
	const FVector StrafeDirection = FromTarget.RotateAngleAxis(SignedArcAngle, FVector::UpVector);
	const FVector RawDestination = ActiveTarget->GetActorLocation() + (StrafeDirection * StrafeRadius);

	if (UWorld* World = GetWorld())
	{
		if (UNavigationSystemV1* NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
		{
			FNavLocation ProjectedLocation;
			if (NavigationSystem->ProjectPointToNavigation(
				RawDestination,
				ProjectedLocation,
				PhaseConfig.StrafeNavProjectExtent))
			{
				OutDestination = ProjectedLocation.Location;
				return true;
			}
		}
	}

	OutDestination = RawDestination;
	return true;
}

void UPRFaerinCombatDirectorComponent::HandlePostPatternStrafeElapsed()
{
	if (AAIController* AIController = GetOwnerAIController())
	{
		AIController->StopMovement();
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
	}

	if (IsValid(OwnerEnemy))
	{
		OwnerEnemy->ClearCombatMovePresentationContext();
	}

	const FPRFaerinPhaseLoopConfig* PhaseConfig = IsValid(LoopDataAsset)
		? LoopDataAsset->FindPhaseConfig(ResolveCurrentPhase())
		: nullptr;
	if (PhaseConfig != nullptr && StartPostStrafeApproach(*PhaseConfig))
	{
		return;
	}

	FinishLoopStep(true);
}

bool UPRFaerinCombatDirectorComponent::ShouldRunPostStrafeApproach(
	const FPRFaerinPatternPlan& PatternPlan,
	const FPRFaerinPhaseLoopConfig& PhaseConfig) const
{
	if (!PhaseConfig.bUsePostStrafeApproach || !IsValid(ActiveTarget))
	{
		return false;
	}

	const EPRFaerinApproachPolicy ApproachPolicy = ResolveApproachPolicy(PatternPlan, PhaseConfig);
	if (ApproachPolicy == EPRFaerinApproachPolicy::None
		|| ApproachPolicy == EPRFaerinApproachPolicy::ShiftClose)
	{
		return false;
	}

	const AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor))
	{
		return false;
	}

	const float DistanceToTarget = FVector::Dist2D(OwnerActor->GetActorLocation(), ActiveTarget->GetActorLocation());
	if (DistanceToTarget > PatternPlan.PatternRule.MaxRange)
	{
		return true;
	}

	if (ApproachPolicy == EPRFaerinApproachPolicy::KeepCurrentRange)
	{
		return false;
	}

	return DistanceToTarget > PhaseConfig.ApproachTriggerDistance;
}

bool UPRFaerinCombatDirectorComponent::StartPostStrafeApproach(const FPRFaerinPhaseLoopConfig& PhaseConfig)
{
	if (IsApproachSprintRepeatBlocked())
	{
		return false;
	}

	if (!ShouldRunPostStrafeApproach(ActivePatternPlan, PhaseConfig))
	{
		return false;
	}

	if (!PhaseConfig.ApproachAbilityTag.IsValid() || !IsValid(AbilitySystemComponent))
	{
		return false;
	}

	FPRFaerinApproachSprintRequest Request;
	if (!BuildApproachSprintRequest(PhaseConfig, Request))
	{
		return false;
	}

	LoopState = EPRFaerinCombatLoopState::ApproachSprint;
	ActiveApproachSprintRequest = Request;

	if (!AbilitySystemComponent->TryActivateAbilityOnServer(PhaseConfig.ApproachAbilityTag, ActiveAbilityHandle))
	{
		UE_LOG(
			LogPRFaerinCombatDirector,
			Warning,
			TEXT("Faerin sprint 접근 Ability 활성화에 실패했다. Owner=%s, AbilityTag=%s"),
			*GetNameSafe(GetOwner()),
			*PhaseConfig.ApproachAbilityTag.ToString());
		ClearActiveApproachSprintRequest();
		return false;
	}

	ObservedAbilityRole = EPRFaerinObservedAbilityRole::Approach;
	BindAbilityEndDelegate();

	if (IsObservedAbilityActive())
	{
		return true;
	}

	QueueFinishLoopStep(true);
	return true;
}

bool UPRFaerinCombatDirectorComponent::BuildApproachSprintRequest(
	const FPRFaerinPhaseLoopConfig& PhaseConfig,
	FPRFaerinApproachSprintRequest& OutRequest) const
{
	OutRequest = FPRFaerinApproachSprintRequest();

	if (!IsValid(GetOwner()) || !IsValid(ActiveTarget))
	{
		return false;
	}

	const EPRFaerinApproachPolicy ApproachPolicy = ResolveApproachPolicy(ActivePatternPlan, PhaseConfig);
	float StopDistance = PhaseConfig.ApproachStopDistance;
	if (ApproachPolicy == EPRFaerinApproachPolicy::KeepCurrentRange)
	{
		StopDistance = FMath::Clamp(
			ActivePatternPlan.PatternRule.MaxRange * 0.85f,
			ActivePatternPlan.PatternRule.MinRange,
			ActivePatternPlan.PatternRule.MaxRange);
	}

	OutRequest.bIsValid = true;
	OutRequest.TargetActor = ActiveTarget;
	OutRequest.TriggerDistance = PhaseConfig.ApproachTriggerDistance;
	OutRequest.StopDistance = StopDistance;
	OutRequest.TimeoutSeconds = PhaseConfig.ApproachTimeoutSeconds;
	OutRequest.AcceptanceRadius = PhaseConfig.ApproachAcceptanceRadius;
	OutRequest.NavProjectExtent = PhaseConfig.ApproachNavProjectExtent;
	OutRequest.PresentationConfig = PhaseConfig.ApproachPresentationConfig;
	return true;
}

void UPRFaerinCombatDirectorComponent::ClearActiveApproachSprintRequest()
{
	ActiveApproachSprintRequest = FPRFaerinApproachSprintRequest();
}

EPRFaerinApproachPolicy UPRFaerinCombatDirectorComponent::ResolveApproachPolicy(
	const FPRFaerinPatternPlan& PatternPlan,
	const FPRFaerinPhaseLoopConfig& PhaseConfig) const
{
	if (PatternPlan.LoopMetadata.ApproachPolicy == EPRFaerinApproachPolicy::PhaseDefault)
	{
		return PhaseConfig.DefaultApproachPolicy;
	}

	return PatternPlan.LoopMetadata.ApproachPolicy;
}

bool UPRFaerinCombatDirectorComponent::IsApproachSprintRepeatBlocked() const
{
	if (ApproachSprintRepeatBlockSeconds <= 0.0f)
	{
		return false;
	}

	const UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return false;
	}

	return World->GetTimeSeconds() - LastApproachSprintEndTime < ApproachSprintRepeatBlockSeconds;
}

void UPRFaerinCombatDirectorComponent::HandleLoopStepFailSafeElapsed()
{
	UE_LOG(
		LogPRFaerinCombatDirector,
		Warning,
		TEXT("Faerin 전투 루프가 제한 시간을 초과했다. Owner=%s, State=%s, AbilityTag=%s"),
		*GetNameSafe(GetOwner()),
		*StaticEnum<EPRFaerinCombatLoopState>()->GetNameStringByValue(static_cast<int64>(LoopState)),
		ActivePatternPlan.AbilityTag.IsValid() ? *ActivePatternPlan.AbilityTag.ToString() : TEXT("None"));

	if (AAIController* AIController = GetOwnerAIController())
	{
		AIController->StopMovement();
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
	}

	if (IsValid(OwnerEnemy))
	{
		OwnerEnemy->ClearCombatMovePresentationContext();
	}

	FinishLoopStep(false);
}

void UPRFaerinCombatDirectorComponent::FinishLoopStep(bool bSucceeded)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LoopStepFailSafeTimerHandle);
		World->GetTimerManager().ClearTimer(PostPatternStrafeTimerHandle);
	}

	ClearAbilityEndDelegate();

	ActivePatternPlan = FPRFaerinPatternPlan();
	ClearActiveApproachSprintRequest();
	ActiveAbilityHandle = FGameplayAbilitySpecHandle();
	ObservedAbilityRole = EPRFaerinObservedAbilityRole::None;
	ActiveTarget = nullptr;
	LoopState = EPRFaerinCombatLoopState::Idle;

	OnFaerinLoopStepFinished.Broadcast(bSucceeded);
}

void UPRFaerinCombatDirectorComponent::QueueFinishLoopStep(bool bSucceeded)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(
			this,
			&UPRFaerinCombatDirectorComponent::FinishLoopStep,
			bSucceeded));
		return;
	}

	FinishLoopStep(bSucceeded);
}

void UPRFaerinCombatDirectorComponent::LogValidationErrors(const TArray<FString>& Errors) const
{
	for (const FString& Error : Errors)
	{
		UE_LOG(
			LogPRFaerinCombatDirector,
			Warning,
			TEXT("Faerin LoopData 검증: %s"),
			*Error);
	}
}

float UPRFaerinCombatDirectorComponent::ResolveHealthRatio() const
{
	if (!IsValid(AbilitySystemComponent))
	{
		return 1.0f;
	}

	const float CurrentHealth = AbilitySystemComponent->GetNumericAttribute(UPRAttributeSet_Common::GetHealthAttribute());
	const float MaxHealth = AbilitySystemComponent->GetNumericAttribute(UPRAttributeSet_Common::GetMaxHealthAttribute());
	if (MaxHealth <= UE_SMALL_NUMBER)
	{
		return 1.0f;
	}

	return FMath::Clamp(CurrentHealth / MaxHealth, 0.0f, 1.0f);
}

EPRBossPhase UPRFaerinCombatDirectorComponent::ResolveCurrentPhase() const
{
	const APRBossBaseCharacter* BossOwner = Cast<APRBossBaseCharacter>(GetOwner());
	return IsValid(BossOwner)
		? BossOwner->GetCurrentPhase()
		: EPRBossPhase::Phase1;
}

void UPRFaerinCombatDirectorComponent::LogSelectedPattern(
	const FPRFaerinPatternPlan& PatternPlan,
	const FPREnemyPatternQueryRuntime& PatternRuntime) const
{
	if (!PREnemyAIDebug::IsPatternLogEnabled())
	{
		return;
	}

	UE_LOG(
		LogPRFaerinCombatDirector,
		Verbose,
		TEXT("Faerin 패턴 선택. Owner=%s, AbilityTag=%s, Phase=%s, Distance=%.1f, FinalWeight=%.2f"),
		*GetNameSafe(PatternRuntime.ControlledPawn),
		*PatternPlan.AbilityTag.ToString(),
		*StaticEnum<EPRBossPhase>()->GetNameStringByValue(static_cast<int64>(PatternRuntime.PatternContext.BossPhase)),
		PatternRuntime.PatternContext.DistanceToTarget,
		PatternPlan.FinalSelectionWeight);
}

AAIController* UPRFaerinCombatDirectorComponent::GetOwnerAIController() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	return IsValid(OwnerPawn)
		? Cast<AAIController>(OwnerPawn->GetController())
		: nullptr;
}

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
	ActiveAbilityHandle = FGameplayAbilitySpecHandle();
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

	HandlePatternExecutionFinished(!EndedData.bWasCancelled);
}

void UPRFaerinCombatDirectorComponent::HandlePatternExecutionFinished(bool bSucceeded)
{
	ClearAbilityEndDelegate();
	ActiveAbilityHandle = FGameplayAbilitySpecHandle();

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
		&& ShouldRunPostPatternStrafe(ActivePatternPlan, *PhaseConfig)
		&& StartPostPatternStrafe(*PhaseConfig))
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

	FinishLoopStep(true);
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
	ActiveAbilityHandle = FGameplayAbilitySpecHandle();
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

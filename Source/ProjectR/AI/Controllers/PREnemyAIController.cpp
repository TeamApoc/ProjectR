// Copyright ProjectR. All Rights Reserved.

#include "PREnemyAIController.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Sight.h"
#include "ProjectR/AI/PREnemyAIDebug.h"
#include "ProjectR/AI/Components/PREnemyThreatComponent.h"
#include "ProjectR/AI/Data/PREnemyCombatDataAsset.h"
#include "ProjectR/AI/Data/PRPerceptionConfig.h"
#include "ProjectR/Character/Enemy/PREnemyInterface.h"

namespace
{
	const UPREnemyCombatDataAsset* ResolveCombatDataAssetFromPawn(const APawn* ControlledPawn)
	{
		const IPREnemyInterface* EnemyInterface = Cast<IPREnemyInterface>(ControlledPawn);
		if (EnemyInterface == nullptr)
		{
			return nullptr;
		}

		return EnemyInterface->GetCombatDataAsset();
	}
}

APREnemyAIController::APREnemyAIController()
{
	EnemyPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComponent"));
	SetPerceptionComponent(*EnemyPerceptionComponent);

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;

	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;

	EnemyPerceptionComponent->ConfigureSense(*SightConfig);
	EnemyPerceptionComponent->ConfigureSense(*HearingConfig);
	EnemyPerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());
	EnemyPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &APREnemyAIController::HandleTargetPerceptionUpdated);
}

void APREnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	IPREnemyInterface* EnemyInterface = Cast<IPREnemyInterface>(InPawn);
	if (EnemyInterface == nullptr)
	{
		return;
	}

	ApplyPerceptionConfig(EnemyInterface->GetPerceptionConfig());

	CachedThreatComponent = EnemyInterface->GetEnemyThreatComponent();
	if (IsValid(CachedThreatComponent))
	{
		CachedThreatComponent->OnTargetChanged.AddDynamic(this, &APREnemyAIController::HandleThreatTargetChanged);
	}

	CacheBlackboardFromBehaviorTree(EnemyInterface->GetBehaviorTreeAsset());

	if (IsValid(CachedBlackboardComponent))
	{
		CachedBlackboardComponent->SetValueAsVector(HomeLocationKey, EnemyInterface->GetHomeLocation());
		ApplyTacticalModeState(EPRTacticalMode::Idle);
	}

	bPreserveAlertOnNextTargetClear = false;
}

void APREnemyAIController::OnUnPossess()
{
	ClearCombatMovePresentationContext(true);
	ClearThreatBinding();
	CachedBlackboardComponent = nullptr;
	bPreserveAlertOnNextTargetClear = false;

	Super::OnUnPossess();
}

void APREnemyAIController::ApplyTacticalModeState(
	EPRTacticalMode NewMode,
	AActor* FocusTarget,
	const FPREnemyMovePresentationConfig* OverridePresentationConfig)
{
	SetBlackboardTacticalMode(NewMode);
	ApplyPresentationForTacticalMode(NewMode, FocusTarget, OverridePresentationConfig);

	if (PREnemyAIDebug::IsPresentationLogEnabled())
	{
		UE_LOG(
			LogPREnemyAI,
			Verbose,
			TEXT("[Presentation] ApplyTacticalMode Mode=%s Override=%s Focus=%s Pawn=%s"),
			*StaticEnum<EPRTacticalMode>()->GetNameStringByValue(static_cast<int64>(NewMode)),
			OverridePresentationConfig != nullptr ? TEXT("true") : TEXT("false"),
			*GetNameSafe(FocusTarget),
			*GetNameSafe(GetPawn()));
	}
}

void APREnemyAIController::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!HasAuthority() || !IsValid(Actor))
	{
		return;
	}

	if (IsValid(CachedBlackboardComponent))
	{
		CachedBlackboardComponent->SetValueAsVector(TargetLocationKey, Stimulus.StimulusLocation);
	}

	if (!IsValid(CachedThreatComponent))
	{
		return;
	}

	if (Stimulus.WasSuccessfullySensed())
	{
		CachedThreatComponent->AddThreat(Actor, 1.0f);

		if (IsValid(CachedBlackboardComponent))
		{
			CachedBlackboardComponent->SetValueAsVector(LastKnownTargetLocationKey, Stimulus.StimulusLocation);
			CachedBlackboardComponent->SetValueAsBool(HasLOSKey, true);
		}

		bPreserveAlertOnNextTargetClear = false;
	}
	else
	{
		if (IsValid(CachedBlackboardComponent))
		{
			CachedBlackboardComponent->SetValueAsVector(LastKnownTargetLocationKey, Stimulus.StimulusLocation);
			CachedBlackboardComponent->SetValueAsBool(HasLOSKey, false);
		}

		switch (TargetLostPolicy)
		{
		case EPRTargetLostPolicy::ClearCurrentTarget:
			bPreserveAlertOnNextTargetClear = true;
			CachedThreatComponent->ReleaseCurrentTargetForSearch(Actor);
			break;
		case EPRTargetLostPolicy::RemoveThreatEntry:
			bPreserveAlertOnNextTargetClear = false;
			if (IsValid(CachedBlackboardComponent))
			{
				CachedBlackboardComponent->ClearValue(LastKnownTargetLocationKey);
			}
			CachedThreatComponent->OnTargetLost(Actor);
			break;
		case EPRTargetLostPolicy::KeepCurrentTarget:
		default:
			bPreserveAlertOnNextTargetClear = false;
			break;
		}
	}
}

void APREnemyAIController::HandleThreatTargetChanged(AActor* OldTarget, AActor* NewTarget)
{
	if (!IsValid(CachedBlackboardComponent))
	{
		return;
	}

	CachedBlackboardComponent->SetValueAsObject(CurrentTargetKey, NewTarget);

	if (IsValid(NewTarget))
	{
		const bool bHasPreviousTarget = IsValid(OldTarget);

		CachedBlackboardComponent->SetValueAsVector(TargetLocationKey, NewTarget->GetActorLocation());
		CachedBlackboardComponent->SetValueAsBool(HasLOSKey, true);
		if (!bHasPreviousTarget)
		{
			ApplyTacticalModeState(EPRTacticalMode::Alert, NewTarget);
			ApplyInitialAttackPressureOnAlert();
		}
		else
		{
			ApplyPresentationForTacticalMode(GetBlackboardTacticalMode(), NewTarget);
		}

		bPreserveAlertOnNextTargetClear = false;
	}
	else
	{
		CachedBlackboardComponent->ClearValue(CurrentTargetKey);
		CachedBlackboardComponent->SetValueAsBool(HasLOSKey, false);

		const bool bShouldInvestigate = bPreserveAlertOnNextTargetClear && HasLastKnownTargetLocation();
		ApplyTacticalModeState(bShouldInvestigate ? EPRTacticalMode::Alert : EPRTacticalMode::Return);
		bPreserveAlertOnNextTargetClear = false;
	}
}

void APREnemyAIController::ApplyPerceptionConfig(const UPRPerceptionConfig* Config)
{
	if (!IsValid(SightConfig) || !IsValid(HearingConfig) || !IsValid(EnemyPerceptionComponent))
	{
		return;
	}

	const float SightRadius = IsValid(Config) ? Config->SightRadius : 1500.0f;
	const float LoseSightRadius = IsValid(Config) ? Config->LoseSightRadius : 1800.0f;
	const float PeripheralVisionAngle = IsValid(Config) ? Config->PeripheralVisionAngle : 90.0f;
	const float HearingRange = IsValid(Config) ? Config->HearingRange : 800.0f;
	const float StimulusMaxAge = IsValid(Config) ? Config->StimulusMaxAge : 5.0f;

	SightConfig->SightRadius = SightRadius;
	SightConfig->LoseSightRadius = LoseSightRadius;
	SightConfig->PeripheralVisionAngleDegrees = PeripheralVisionAngle;
	SightConfig->SetMaxAge(StimulusMaxAge);

	HearingConfig->HearingRange = HearingRange;
	HearingConfig->SetMaxAge(StimulusMaxAge);

	EnemyPerceptionComponent->ConfigureSense(*SightConfig);
	EnemyPerceptionComponent->ConfigureSense(*HearingConfig);
	EnemyPerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());
}

void APREnemyAIController::ApplyCombatMovePresentationContext(
	AActor* FocusTarget,
	const FPREnemyMovePresentationConfig& PresentationConfig)
{
	if (APawn* ControlledPawn = GetPawn())
	{
		if (IPREnemyInterface* EnemyInterface = Cast<IPREnemyInterface>(ControlledPawn))
		{
			EnemyInterface->ApplyCombatMovePresentationContext(PresentationConfig);
		}
	}

	if (PresentationConfig.bMaintainTargetFocus && IsValid(FocusTarget))
	{
		SetFocus(FocusTarget);
	}
	else
	{
		ClearFocus(EAIFocusPriority::Gameplay);
	}

	if (PREnemyAIDebug::IsPresentationLogEnabled())
	{
		UE_LOG(
			LogPREnemyAI,
			Verbose,
			TEXT("[Presentation] Apply MoveSpeed=%.1f YawRate=%.1f Focus=%s CombatMove=%s AimOffset=%s StrafeState=%s Pawn=%s"),
			PresentationConfig.MoveSpeedOverride,
			PresentationConfig.RotationYawRate,
			PresentationConfig.bMaintainTargetFocus ? TEXT("true") : TEXT("false"),
			PresentationConfig.bUseCombatMovePose ? TEXT("true") : TEXT("false"),
			PresentationConfig.bUseCombatAimOffset ? TEXT("true") : TEXT("false"),
			PresentationConfig.bUseCombatStrafeState ? TEXT("true") : TEXT("false"),
			*GetNameSafe(GetPawn()));
	}
}

void APREnemyAIController::ClearCombatMovePresentationContext(bool bClearGameplayFocus)
{
	if (APawn* ControlledPawn = GetPawn())
	{
		if (IPREnemyInterface* EnemyInterface = Cast<IPREnemyInterface>(ControlledPawn))
		{
			EnemyInterface->ClearCombatMovePresentationContext();
		}
	}

	if (bClearGameplayFocus)
	{
		ClearFocus(EAIFocusPriority::Gameplay);
	}

	if (PREnemyAIDebug::IsPresentationLogEnabled())
	{
		UE_LOG(
			LogPREnemyAI,
			Verbose,
			TEXT("[Presentation] Clear Focus=%s Pawn=%s"),
			bClearGameplayFocus ? TEXT("true") : TEXT("false"),
			*GetNameSafe(GetPawn()));
	}
}

EPRTacticalMode APREnemyAIController::GetBlackboardTacticalMode() const
{
	if (!IsValid(CachedBlackboardComponent))
	{
		return EPRTacticalMode::Idle;
	}

	return static_cast<EPRTacticalMode>(CachedBlackboardComponent->GetValueAsEnum(TacticalModeKey));
}

bool APREnemyAIController::HasLastKnownTargetLocation() const
{
	return IsValid(CachedBlackboardComponent)
		&& CachedBlackboardComponent->IsVectorValueSet(LastKnownTargetLocationKey);
}

void APREnemyAIController::SetBlackboardTacticalMode(EPRTacticalMode NewMode)
{
	if (!IsValid(CachedBlackboardComponent))
	{
		return;
	}

	CachedBlackboardComponent->SetValueAsEnum(TacticalModeKey, static_cast<uint8>(NewMode));
}

const UPREnemyCombatDataAsset* APREnemyAIController::GetCurrentCombatDataAsset() const
{
	return ResolveCombatDataAssetFromPawn(GetPawn());
}

void APREnemyAIController::ApplyPresentationForTacticalMode(
	EPRTacticalMode NewMode,
	AActor* FocusTarget,
	const FPREnemyMovePresentationConfig* OverridePresentationConfig)
{
	AActor* ResolvedFocusTarget = FocusTarget;
	if (!IsValid(ResolvedFocusTarget) && IsValid(CachedBlackboardComponent))
	{
		ResolvedFocusTarget = Cast<AActor>(CachedBlackboardComponent->GetValueAsObject(CurrentTargetKey));
	}

	if (OverridePresentationConfig != nullptr)
	{
		ApplyCombatMovePresentationContext(ResolvedFocusTarget, *OverridePresentationConfig);
		return;
	}

	const UPREnemyCombatDataAsset* CombatDataAsset = GetCurrentCombatDataAsset();
	if (!IsValid(CombatDataAsset))
	{
		ClearCombatMovePresentationContext(true);
		return;
	}

	const FPREnemyMovePresentationConfig* PresentationConfig = CombatDataAsset->FindTacticalModePresentationConfig(NewMode);
	if (PresentationConfig == nullptr)
	{
		ClearCombatMovePresentationContext(true);
		return;
	}

	ApplyCombatMovePresentationContext(ResolvedFocusTarget, *PresentationConfig);
}

void APREnemyAIController::ApplyInitialAttackPressureOnAlert()
{
	if (!IsValid(CachedBlackboardComponent))
	{
		return;
	}

	const UPREnemyCombatDataAsset* CombatDataAsset = GetCurrentCombatDataAsset();
	if (!IsValid(CombatDataAsset) || AttackPressureKey == NAME_None)
	{
		return;
	}

	if (CachedBlackboardComponent->GetKeyID(AttackPressureKey) == FBlackboard::InvalidKey)
	{
		return;
	}

	const float InitialAttackPressure = FMath::Clamp(
		CombatDataAsset->AttackPressureConfig.InitialAttackPressureOnAlert,
		0.0f,
		CombatDataAsset->AttackPressureConfig.MaxPressure);

	CachedBlackboardComponent->SetValueAsFloat(AttackPressureKey, InitialAttackPressure);

	if (PREnemyAIDebug::IsAttackPressureLogEnabled())
	{
		UE_LOG(
			LogPREnemyAI,
			Verbose,
			TEXT("[AttackPressure] AlertInitial Value=%.2f Pawn=%s"),
			InitialAttackPressure,
			*GetNameSafe(GetPawn()));
	}
}

void APREnemyAIController::CacheBlackboardFromBehaviorTree(UBehaviorTree* BehaviorTreeAsset)
{
	CachedBlackboardComponent = nullptr;

	if (!IsValid(BehaviorTreeAsset))
	{
		return;
	}

	UBlackboardComponent* BlackboardComponent = nullptr;
	const bool bBlackboardReady = UseBlackboard(BehaviorTreeAsset->BlackboardAsset, BlackboardComponent);
	if (!bBlackboardReady || !IsValid(BlackboardComponent))
	{
		return;
	}

	CachedBlackboardComponent = BlackboardComponent;
	RunBehaviorTree(BehaviorTreeAsset);
}

void APREnemyAIController::ClearThreatBinding()
{
	if (IsValid(CachedThreatComponent))
	{
		CachedThreatComponent->OnTargetChanged.RemoveAll(this);
		CachedThreatComponent = nullptr;
	}
}

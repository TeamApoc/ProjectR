// Copyright ProjectR. All Rights Reserved.

#include "PREnemyAIController.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Sight.h"
#include "ProjectR/AI/Components/PREnemyThreatComponent.h"
#include "ProjectR/AI/Data/PRPerceptionConfig.h"
#include "ProjectR/Character/Enemy/PREnemyInterface.h"

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
		SetBlackboardTacticalMode(EPRTacticalMode::Idle);
	}
}

void APREnemyAIController::OnUnPossess()
{
	ClearThreatBinding();
	CachedBlackboardComponent = nullptr;

	Super::OnUnPossess();
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

		SetBlackboardTacticalMode(EPRTacticalMode::Alert);
	}
	else
	{
		if (IsValid(CachedBlackboardComponent))
		{
			CachedBlackboardComponent->SetValueAsVector(LastKnownTargetLocationKey, Stimulus.StimulusLocation);
			CachedBlackboardComponent->SetValueAsBool(HasLOSKey, false);
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
		CachedBlackboardComponent->SetValueAsVector(TargetLocationKey, NewTarget->GetActorLocation());
		CachedBlackboardComponent->SetValueAsBool(HasLOSKey, true);
		SetFocus(NewTarget);
		SetBlackboardTacticalMode(EPRTacticalMode::Chase);
	}
	else
	{
		CachedBlackboardComponent->ClearValue(CurrentTargetKey);
		CachedBlackboardComponent->SetValueAsBool(HasLOSKey, false);
		ClearFocus(EAIFocusPriority::Gameplay);
		SetBlackboardTacticalMode(EPRTacticalMode::Return);
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

void APREnemyAIController::SetBlackboardTacticalMode(EPRTacticalMode NewMode)
{
	if (!IsValid(CachedBlackboardComponent))
	{
		return;
	}

	CachedBlackboardComponent->SetValueAsEnum(TacticalModeKey, static_cast<uint8>(NewMode));
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

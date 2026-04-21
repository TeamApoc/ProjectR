// Copyright ProjectR. All Rights Reserved.

#include "BTTask_PRSelectEnemyPattern.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "ProjectR/AI/Data/PRPatternDataAsset.h"
#include "ProjectR/Character/Enemy/PREnemyInterface.h"

UBTTask_PRSelectEnemyPattern::UBTTask_PRSelectEnemyPattern()
{
	NodeName = TEXT("Select Enemy Pattern");
}

EBTNodeResult::Type UBTTask_PRSelectEnemyPattern::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	const AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* ControlledPawn = IsValid(AIController) ? AIController->GetPawn() : nullptr;
	IPREnemyInterface* EnemyInterface = Cast<IPREnemyInterface>(ControlledPawn);
	if (!IsValid(BlackboardComponent) || !IsValid(ControlledPawn) || EnemyInterface == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	UPRPatternDataAsset* PatternDataAsset = EnemyInterface->GetPatternDataAsset();
	AActor* CurrentTarget = Cast<AActor>(BlackboardComponent->GetValueAsObject(CurrentTargetKey));
	if (!IsValid(PatternDataAsset) || !IsValid(CurrentTarget))
	{
		return EBTNodeResult::Failed;
	}

	FPRPatternContext PatternContext;
	PatternContext.DistanceToTarget = FVector::Dist(ControlledPawn->GetActorLocation(), CurrentTarget->GetActorLocation());
	PatternContext.bHasLOS = BlackboardComponent->GetValueAsBool(HasLOSKey);
	PatternContext.TacticalMode = static_cast<EPRTacticalMode>(BlackboardComponent->GetValueAsEnum(TacticalModeKey));

	TArray<const FPRPatternRule*> MatchedRules;
	float TotalWeight = 0.0f;

	for (const FPRPatternRule& PatternRule : PatternDataAsset->PatternRules)
	{
		if (PatternRule.MatchesContext(PatternContext))
		{
			MatchedRules.Add(&PatternRule);
			TotalWeight += PatternRule.SelectionWeight;
		}
	}

	if (MatchedRules.IsEmpty() || TotalWeight <= 0.0f)
	{
		return EBTNodeResult::Failed;
	}

	const float PickValue = FMath::FRandRange(0.0f, TotalWeight);
	float AccumulatedWeight = 0.0f;

	for (const FPRPatternRule* PatternRule : MatchedRules)
	{
		AccumulatedWeight += PatternRule->SelectionWeight;
		if (PickValue <= AccumulatedWeight)
		{
			BlackboardComponent->SetValueAsName(SelectedAbilityTagKey, PatternRule->AbilityTag.GetTagName());
			return EBTNodeResult::Succeeded;
		}
	}

	BlackboardComponent->SetValueAsName(SelectedAbilityTagKey, MatchedRules.Last()->AbilityTag.GetTagName());
	return EBTNodeResult::Succeeded;
}

FString UBTTask_PRSelectEnemyPattern::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s\nWrite Key: %s"),
		*Super::GetStaticDescription(),
		*SelectedAbilityTagKey.ToString());
}

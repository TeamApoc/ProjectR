// Copyright ProjectR. All Rights Reserved.

#include "BTTask_PRSelectEnemyPattern.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "ProjectR/AI/PREnemyAIDebug.h"
#include "ProjectR/AI/PREnemyPatternSelectionUtils.h"
#include "ProjectR/AI/Controllers/PREnemyAIController.h"

UBTTask_PRSelectEnemyPattern::UBTTask_PRSelectEnemyPattern()
{
	NodeName = TEXT("Select Enemy Pattern");
}

EBTNodeResult::Type UBTTask_PRSelectEnemyPattern::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	FPREnemyPatternQueryRuntime PatternRuntime;
	if (!IsValid(BlackboardComponent)
		|| !PREnemyPatternSelectionUtils::BuildPatternQueryRuntime(
			OwnerComp,
			CurrentTargetKey,
			HasLOSKey,
			ChargePathClearKey,
			TacticalModeKey,
			AttackPressureKey,
			PatternRuntime))
	{
		return EBTNodeResult::Failed;
	}

	TArray<const FPRPatternRule*> MatchedRules;
	float TotalWeight = 0.0f;
	PREnemyPatternSelectionUtils::CollectMatchingPatternRules(
		PatternRuntime,
		PatternCategoryFilter,
		bCheckAbilityCanActivate,
		EPRPatternContextMatchMode::FullMatch,
		MatchedRules,
		&TotalWeight);

	if (MatchedRules.IsEmpty() || TotalWeight <= 0.0f)
	{
		if (PREnemyAIDebug::IsPatternLogEnabled())
		{
			UE_LOG(
				LogPREnemyAI,
				Verbose,
				TEXT("[Pattern] NoCandidate Category=%s Distance=%.1f Pressure=%.2f Pawn=%s"),
				*StaticEnum<EPRPatternCategory>()->GetNameStringByValue(static_cast<int64>(PatternCategoryFilter)),
				PatternRuntime.PatternContext.DistanceToTarget,
				PatternRuntime.PatternContext.CurrentAttackPressure,
				*GetNameSafe(PatternRuntime.ControlledPawn));
		}
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
			if (PREnemyAIDebug::IsPatternLogEnabled())
			{
				UE_LOG(
					LogPREnemyAI,
					Verbose,
					TEXT("[Pattern] Selected Category=%s Tag=%s Distance=%.1f Pressure=%.2f Pawn=%s"),
					*StaticEnum<EPRPatternCategory>()->GetNameStringByValue(static_cast<int64>(PatternCategoryFilter)),
					*PatternRule->AbilityTag.ToString(),
					PatternRuntime.PatternContext.DistanceToTarget,
					PatternRuntime.PatternContext.CurrentAttackPressure,
					*GetNameSafe(PatternRuntime.ControlledPawn));
			}

			if (bSetTacticalModeOnSelection && PREnemyPatternSelectionUtils::HasValidBlackboardKey(BlackboardComponent, TacticalModeKey))
			{
				if (APREnemyAIController* EnemyAIController = Cast<APREnemyAIController>(OwnerComp.GetAIOwner()))
				{
					EnemyAIController->ApplyTacticalModeState(TacticalModeOnSelection, PatternRuntime.CurrentTarget);
				}
				else
				{
					BlackboardComponent->SetValueAsEnum(TacticalModeKey, static_cast<uint8>(TacticalModeOnSelection));
				}
			}

			return EBTNodeResult::Succeeded;
		}
	}

	BlackboardComponent->SetValueAsName(SelectedAbilityTagKey, MatchedRules.Last()->AbilityTag.GetTagName());
	if (PREnemyAIDebug::IsPatternLogEnabled())
	{
		UE_LOG(
			LogPREnemyAI,
			Verbose,
			TEXT("[Pattern] SelectedFallback Category=%s Tag=%s Distance=%.1f Pressure=%.2f Pawn=%s"),
			*StaticEnum<EPRPatternCategory>()->GetNameStringByValue(static_cast<int64>(PatternCategoryFilter)),
			*MatchedRules.Last()->AbilityTag.ToString(),
			PatternRuntime.PatternContext.DistanceToTarget,
			PatternRuntime.PatternContext.CurrentAttackPressure,
			*GetNameSafe(PatternRuntime.ControlledPawn));
	}

	if (bSetTacticalModeOnSelection && PREnemyPatternSelectionUtils::HasValidBlackboardKey(BlackboardComponent, TacticalModeKey))
	{
		if (APREnemyAIController* EnemyAIController = Cast<APREnemyAIController>(OwnerComp.GetAIOwner()))
		{
			EnemyAIController->ApplyTacticalModeState(TacticalModeOnSelection, PatternRuntime.CurrentTarget);
		}
		else
		{
			BlackboardComponent->SetValueAsEnum(TacticalModeKey, static_cast<uint8>(TacticalModeOnSelection));
		}
	}

	return EBTNodeResult::Succeeded;
}

FString UBTTask_PRSelectEnemyPattern::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s\nCategory: %s\nWrite Key: %s"),
		*Super::GetStaticDescription(),
		*StaticEnum<EPRPatternCategory>()->GetNameStringByValue(static_cast<int64>(PatternCategoryFilter)),
		*SelectedAbilityTagKey.ToString());
}

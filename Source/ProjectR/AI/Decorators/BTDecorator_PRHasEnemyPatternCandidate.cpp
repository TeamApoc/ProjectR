// Copyright ProjectR. All Rights Reserved.

#include "BTDecorator_PRHasEnemyPatternCandidate.h"

#include "ProjectR/AI/PREnemyPatternSelectionUtils.h"

UBTDecorator_PRHasEnemyPatternCandidate::UBTDecorator_PRHasEnemyPatternCandidate()
{
	NodeName = TEXT("Has Enemy Pattern Candidate");
}

bool UBTDecorator_PRHasEnemyPatternCandidate::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
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
		bCheckAbilityCanActivate,
		MatchMode,
		MatchedRules);

	return !MatchedRules.IsEmpty();
}

FString UBTDecorator_PRHasEnemyPatternCandidate::GetStaticDescription() const
{
	return FString::Printf(
		TEXT("%s\nCategory: %s\nMatchMode: %s"),
		*Super::GetStaticDescription(),
		*StaticEnum<EPRPatternCategory>()->GetNameStringByValue(static_cast<int64>(PatternCategoryFilter)),
		*StaticEnum<EPRPatternContextMatchMode>()->GetNameStringByValue(static_cast<int64>(MatchMode)));
}

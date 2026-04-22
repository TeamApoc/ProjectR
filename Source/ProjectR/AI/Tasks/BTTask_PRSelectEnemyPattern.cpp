// Copyright ProjectR. All Rights Reserved.

#include "BTTask_PRSelectEnemyPattern.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "GameplayTagContainer.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/AI/Data/PRPatternDataAsset.h"
#include "ProjectR/Character/Enemy/PREnemyInterface.h"

namespace
{
	bool HasBlackboardKey(const UBlackboardComponent* BlackboardComponent, const FName KeyName)
	{
		return IsValid(BlackboardComponent)
			&& KeyName != NAME_None
			&& BlackboardComponent->GetKeyID(KeyName) != FBlackboard::InvalidKey;
	}

	bool MatchesCategory(const FPRPatternRule& PatternRule, const EPRPatternCategory CategoryFilter)
	{
		// 기존 데이터 호환을 위해 Any는 양쪽 모두에서 와일드카드로 취급한다.
		return CategoryFilter == EPRPatternCategory::Any
			|| PatternRule.PatternCategory == EPRPatternCategory::Any
			|| PatternRule.PatternCategory == CategoryFilter;
	}
}

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

	UPRAbilitySystemComponent* AbilitySystemComponent = EnemyInterface->GetEnemyAbilitySystemComponent();

	FPRPatternContext PatternContext;
	PatternContext.DistanceToTarget = FVector::Dist(ControlledPawn->GetActorLocation(), CurrentTarget->GetActorLocation());
	PatternContext.bHasLOS = HasBlackboardKey(BlackboardComponent, HasLOSKey)
		? BlackboardComponent->GetValueAsBool(HasLOSKey)
		: false;
	PatternContext.TacticalMode = HasBlackboardKey(BlackboardComponent, TacticalModeKey)
		? static_cast<EPRTacticalMode>(BlackboardComponent->GetValueAsEnum(TacticalModeKey))
		: EPRTacticalMode::Idle;
	PatternContext.bChargePathClear = HasBlackboardKey(BlackboardComponent, ChargePathClearKey)
		? BlackboardComponent->GetValueAsBool(ChargePathClearKey)
		: false;
	PatternContext.ComboIndex = HasBlackboardKey(BlackboardComponent, ComboIndexKey)
		? BlackboardComponent->GetValueAsInt(ComboIndexKey)
		: 0;

	// 현재 상황에 맞는 패턴만 후보로 모은다.
	// 거리/LOS/돌진 경로/콤보 조건은 FPRPatternRule::MatchesContext에서 통일해서 검사한다.
	TArray<const FPRPatternRule*> MatchedRules;
	float TotalWeight = 0.0f;

	for (const FPRPatternRule& PatternRule : PatternDataAsset->PatternRules)
	{
		if (!MatchesCategory(PatternRule, PatternCategoryFilter))
		{
			continue;
		}

		if (!PatternRule.MatchesContext(PatternContext))
		{
			continue;
		}

		if (bCheckAbilityCanActivate && IsValid(AbilitySystemComponent))
		{
			FGameplayTagContainer FailureTags;
			if (!AbilitySystemComponent->CanActivateAbilityByTag(PatternRule.AbilityTag, FailureTags))
			{
				continue;
			}
		}

		MatchedRules.Add(&PatternRule);
		TotalWeight += PatternRule.SelectionWeight;
	}

	if (MatchedRules.IsEmpty() || TotalWeight <= 0.0f)
	{
		return EBTNodeResult::Failed;
	}

	const float PickValue = FMath::FRandRange(0.0f, TotalWeight);
	float AccumulatedWeight = 0.0f;

	// 가중치 랜덤 선택이다. 후보 순서와 상관없이 SelectionWeight 비율로 선택된다.
	for (const FPRPatternRule* PatternRule : MatchedRules)
	{
		AccumulatedWeight += PatternRule->SelectionWeight;
		if (PickValue <= AccumulatedWeight)
		{
			BlackboardComponent->SetValueAsName(SelectedAbilityTagKey, PatternRule->AbilityTag.GetTagName());
			if (HasBlackboardKey(BlackboardComponent, SelectedNextComboIndexKey))
			{
				BlackboardComponent->SetValueAsInt(SelectedNextComboIndexKey, PatternRule->NextComboIndex);
			}
			if (bSetTacticalModeOnSelection && HasBlackboardKey(BlackboardComponent, TacticalModeKey))
			{
				BlackboardComponent->SetValueAsEnum(TacticalModeKey, static_cast<uint8>(TacticalModeOnSelection));
			}
			return EBTNodeResult::Succeeded;
		}
	}

	// 부동소수 오차 등으로 루프 안에서 선택되지 못한 경우 마지막 후보를 안전값으로 쓴다.
	BlackboardComponent->SetValueAsName(SelectedAbilityTagKey, MatchedRules.Last()->AbilityTag.GetTagName());
	if (HasBlackboardKey(BlackboardComponent, SelectedNextComboIndexKey))
	{
		BlackboardComponent->SetValueAsInt(SelectedNextComboIndexKey, MatchedRules.Last()->NextComboIndex);
	}
	if (bSetTacticalModeOnSelection && HasBlackboardKey(BlackboardComponent, TacticalModeKey))
	{
		BlackboardComponent->SetValueAsEnum(TacticalModeKey, static_cast<uint8>(TacticalModeOnSelection));
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

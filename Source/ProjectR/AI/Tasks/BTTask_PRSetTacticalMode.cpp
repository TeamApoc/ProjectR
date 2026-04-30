// Copyright ProjectR. All Rights Reserved.

#include "BTTask_PRSetTacticalMode.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "ProjectR/AI/Controllers/PREnemyAIController.h"

namespace
{
	bool HasSetTacticalModeBlackboardKey(const UBlackboardComponent* BlackboardComponent, const FName KeyName)
	{
		return IsValid(BlackboardComponent)
			&& KeyName != NAME_None
			&& BlackboardComponent->GetKeyID(KeyName) != FBlackboard::InvalidKey;
	}
}

UBTTask_PRSetTacticalMode::UBTTask_PRSetTacticalMode()
{
	NodeName = TEXT("Set Tactical Mode");
}

EBTNodeResult::Type UBTTask_PRSetTacticalMode::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	if (!IsValid(BlackboardComponent) || !HasSetTacticalModeBlackboardKey(BlackboardComponent, TacticalModeKey))
	{
		return EBTNodeResult::Failed;
	}

	AActor* FocusTarget = nullptr;
	if (HasSetTacticalModeBlackboardKey(BlackboardComponent, CurrentTargetKey))
	{
		FocusTarget = Cast<AActor>(BlackboardComponent->GetValueAsObject(CurrentTargetKey));
	}

	if (APREnemyAIController* EnemyAIController = Cast<APREnemyAIController>(OwnerComp.GetAIOwner()))
	{
		EnemyAIController->ApplyTacticalModeState(NewTacticalMode, FocusTarget);
		return EBTNodeResult::Succeeded;
	}

	BlackboardComponent->SetValueAsEnum(TacticalModeKey, static_cast<uint8>(NewTacticalMode));
	return EBTNodeResult::Succeeded;
}

FString UBTTask_PRSetTacticalMode::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s\nMode: %s"),
		*Super::GetStaticDescription(),
		*StaticEnum<EPRTacticalMode>()->GetNameStringByValue(static_cast<int64>(NewTacticalMode)));
}

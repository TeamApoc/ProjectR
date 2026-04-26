// Copyright ProjectR. All Rights Reserved.

#include "BTTask_PRClearBlackboardValue.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"

namespace
{
	bool HasClearBlackboardKey(const UBlackboardComponent* BlackboardComponent, const FName KeyName)
	{
		return IsValid(BlackboardComponent)
			&& KeyName != NAME_None
			&& BlackboardComponent->GetKeyID(KeyName) != FBlackboard::InvalidKey;
	}
}

UBTTask_PRClearBlackboardValue::UBTTask_PRClearBlackboardValue()
{
	NodeName = TEXT("Clear Blackboard Value");
}

EBTNodeResult::Type UBTTask_PRClearBlackboardValue::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	if (!IsValid(BlackboardComponent) || BlackboardKey == NAME_None)
	{
		return EBTNodeResult::Failed;
	}

	if (!HasClearBlackboardKey(BlackboardComponent, BlackboardKey))
	{
		return bFailIfKeyMissing ? EBTNodeResult::Failed : EBTNodeResult::Succeeded;
	}

	BlackboardComponent->ClearValue(BlackboardKey);
	return EBTNodeResult::Succeeded;
}

FString UBTTask_PRClearBlackboardValue::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s\nClear Key: %s"),
		*Super::GetStaticDescription(),
		*BlackboardKey.ToString());
}

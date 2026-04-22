// Copyright ProjectR. All Rights Reserved.

#include "BTTask_PRSetTacticalMode.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"

namespace
{
	bool HasBlackboardKey(const UBlackboardComponent* BlackboardComponent, const FName KeyName)
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
	if (!IsValid(BlackboardComponent) || !HasBlackboardKey(BlackboardComponent, TacticalModeKey))
	{
		return EBTNodeResult::Failed;
	}

	BlackboardComponent->SetValueAsEnum(TacticalModeKey, static_cast<uint8>(NewTacticalMode));

	if (bSetComboIndex && HasBlackboardKey(BlackboardComponent, ComboIndexKey))
	{
		// 수색/복귀처럼 공격 흐름이 끊기는 브랜치에서 콤보를 명시적으로 정리할 때 사용한다.
		BlackboardComponent->SetValueAsInt(ComboIndexKey, ComboIndexValue);
	}

	return EBTNodeResult::Succeeded;
}

FString UBTTask_PRSetTacticalMode::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s\nMode: %s"),
		*Super::GetStaticDescription(),
		*StaticEnum<EPRTacticalMode>()->GetNameStringByValue(static_cast<int64>(NewTacticalMode)));
}

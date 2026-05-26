// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_PRSelectPenitentRangedAbility.generated.h"

// Penitent 원거리 Ability 우선순위 선택 Task
UCLASS()
class PROJECTR_API UBTTask_PRSelectPenitentRangedAbility : public UBTTaskNode
{
	GENERATED_BODY()

public:
	// Task 표시 이름 초기화
	UBTTask_PRSelectPenitentRangedAbility();

	/*~ UBTTaskNode Interface ~*/
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;

protected:
	// 마지막 실행 원거리 AbilityTag 이름 키
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName LastUsedRangedAbilityKey = TEXT("last_used_ranged_ability");

	// 선택한 AbilityTag 이름 기록 키
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName SelectedAbilityTagKey = TEXT("selected_ability_tag");
};

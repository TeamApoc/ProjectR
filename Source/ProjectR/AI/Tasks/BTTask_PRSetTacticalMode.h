// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "ProjectR/AI/PREnemyAITypes.h"
#include "BTTask_PRSetTacticalMode.generated.h"

// BT 브랜치 진입/종료 시 tactical_mode를 명시적으로 바꾸는 Task다.
// 이동, 수색, 복귀처럼 Ability가 아닌 행동도 같은 전술 상태 규약을 쓰게 한다.
UCLASS()
class PROJECTR_API UBTTask_PRSetTacticalMode : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_PRSetTacticalMode();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;

protected:
	// tactical_mode를 저장하는 Blackboard 키다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName TacticalModeKey = TEXT("tactical_mode");

	// 이 Task가 기록할 새 전술 상태다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	EPRTacticalMode NewTacticalMode = EPRTacticalMode::Idle;

	// true면 전술 상태 변경과 함께 콤보 단계를 지정값으로 되돌린다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Combo")
	bool bSetComboIndex = false;

	UPROPERTY(EditAnywhere, Category = "ProjectR|Combo", meta = (EditCondition = "bSetComboIndex"))
	FName ComboIndexKey = TEXT("combo_index");

	UPROPERTY(EditAnywhere, Category = "ProjectR|Combo", meta = (EditCondition = "bSetComboIndex"))
	int32 ComboIndexValue = 0;
};

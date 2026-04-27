// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "ProjectR/AI/PREnemyAITypes.h"
#include "BTTask_PRPickCombatOffsetLocation.generated.h"

class UPREnemyCombatDataAsset;

UENUM(BlueprintType)
enum class EPRCombatOffsetSelectionMode : uint8
{
	Strafe		UMETA(DisplayName = "Strafe"),
	Reposition	UMETA(DisplayName = "Reposition")
};

// 전투 이동용 EQS를 실행해 move_goal_location을 기록하는 Task다.
// 기존 이름은 유지하지만, 내부 로직은 더 이상 수동 오프셋 계산을 사용하지 않는다.
UCLASS()
class PROJECTR_API UBTTask_PRPickCombatOffsetLocation : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_PRPickCombatOffsetLocation();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;

protected:
	// 현재 타겟을 읽을 Blackboard 키다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName CurrentTargetKey = TEXT("current_target");

	// 선택된 이동 목표 지점을 기록할 Blackboard 키다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName MoveGoalLocationKey = TEXT("move_goal_location");

	// 전술 상태를 기록할 Blackboard 키다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName TacticalModeKey = TEXT("tactical_mode");

	// Character 쪽 공용 전투 데이터 대신 사용할 override 자산이다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Combat")
	TObjectPtr<UPREnemyCombatDataAsset> CombatDataAsset;

	// Strafe / Reposition 중 어떤 설정을 사용할지 정한다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Combat")
	EPRCombatOffsetSelectionMode SelectionMode = EPRCombatOffsetSelectionMode::Strafe;

	// 성공 시 tactical_mode를 바꿀지 여부다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Combat")
	bool bSetTacticalModeOnSuccess = true;

	// 성공 시 기록할 전술 상태다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Combat", meta = (EditCondition = "bSetTacticalModeOnSuccess"))
	EPRTacticalMode TacticalModeOnSuccess = EPRTacticalMode::Reposition;
};

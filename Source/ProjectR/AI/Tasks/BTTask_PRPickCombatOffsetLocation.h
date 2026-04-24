// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "ProjectR/AI/PREnemyAITypes.h"
#include "BTTask_PRPickCombatOffsetLocation.generated.h"

class UPRSoldierArmoredCombatDataAsset;

UENUM(BlueprintType)
enum class EPRCombatOffsetSelectionMode : uint8
{
	Strafe		UMETA(DisplayName = "Strafe"),
	Reposition	UMETA(DisplayName = "Reposition")
};

// 타겟 주변 오프셋 지점을 선택해 move_goal_location에 기록하는 Task다.
UCLASS()
class PROJECTR_API UBTTask_PRPickCombatOffsetLocation : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_PRPickCombatOffsetLocation();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;

protected:
	// 현재 타겟을 읽는 Blackboard 키다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName CurrentTargetKey = TEXT("current_target");

	// 선택한 이동 목표 지점을 기록할 Blackboard 키다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName MoveGoalLocationKey = TEXT("move_goal_location");

	// 전술 상태를 기록할 Blackboard 키다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName TacticalModeKey = TEXT("tactical_mode");

	// 오프셋 규칙을 읽을 전투 데이터 에셋이다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Combat")
	TObjectPtr<UPRSoldierArmoredCombatDataAsset> CombatDataAsset;

	// Strafe/Reposition 중 어떤 규칙을 사용할지 정한다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Combat")
	EPRCombatOffsetSelectionMode SelectionMode = EPRCombatOffsetSelectionMode::Strafe;

	// 좌/우 우선 순서를 매번 섞을지 여부다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Combat")
	bool bRandomizeSideOrder = true;

	// 성공 시 tactical_mode를 Reposition으로 바꿀지 여부다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Combat")
	bool bSetTacticalModeOnSuccess = true;

	// 성공 시 기록할 전술 상태다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Combat", meta = (EditCondition = "bSetTacticalModeOnSuccess"))
	EPRTacticalMode TacticalModeOnSuccess = EPRTacticalMode::Reposition;

	// 한쪽 방향에서 위치를 찾을 때 재시도 횟수다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Combat", meta = (ClampMin = "1"))
	int32 AttemptsPerSide = 2;
};

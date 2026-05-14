// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_PRPickReachableLocation.generated.h"

class UBlackboardComponent;
class UNavigationQueryFilter;

// Blackboard 기준 위치 주변에서 NavMesh 이동 가능 지점을 선택하는 공용 Task다.
UCLASS()
class PROJECTR_API UBTTask_PRPickReachableLocation : public UBTTaskNode
{
	GENERATED_BODY()

public:
	// 기본 Blackboard 키와 탐색 설정을 초기화한다.
	UBTTask_PRPickReachableLocation();

	/*~ UBTTaskNode Interface ~*/
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;

protected:
	// 기준 위치 Blackboard 값을 읽는다.
	bool ResolveOriginLocation(const UBlackboardComponent* BlackboardComponent, FVector& OutOriginLocation) const;

	// 기준 위치 주변에서 이동 가능한 NavMesh 지점을 선택한다.
	bool PickReachableLocation(const UObject* WorldContextObject, const FVector& OriginLocation, FVector& OutPickedLocation) const;

	// 선택된 이동 목표를 Blackboard에 기록한다.
	bool WriteGoalLocation(UBlackboardComponent* BlackboardComponent, const FVector& GoalLocation) const;

protected:
	// 순찰/수색 기준 위치를 읽을 Blackboard 키다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName OriginLocationKey = TEXT("home_location");

	// 선택된 이동 지점을 기록할 Blackboard 키다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName MoveGoalLocationKey = TEXT("move_goal_location");

	// 기준 위치 주변에서 이동 지점을 고를 반경이다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Navigation", meta = (ClampMin = "0.0"))
	float PickRadius = 700.0f;

	// 기준 위치와 너무 가까운 후보를 거르기 위한 최소 거리다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Navigation", meta = (ClampMin = "0.0"))
	float MinDistanceFromOrigin = 150.0f;

	// NavMesh 후보를 다시 뽑아볼 최대 횟수다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Navigation", meta = (ClampMin = "1"))
	int32 MaxPickAttempts = 8;

	// NavMesh 후보를 찾지 못했을 때 기준 위치를 목표로 기록할지 여부다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Navigation")
	bool bFallbackToOrigin = true;

	// 이동 가능 지점 검색에 사용할 Navigation Query Filter다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Navigation")
	TSubclassOf<UNavigationQueryFilter> FilterClass;
};

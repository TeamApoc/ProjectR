// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_PRUpdateEnemyCombatBlackboard.generated.h"

// 전투 중 자주 필요한 Blackboard 값을 한 곳에서 갱신하는 서비스다.
// BT 조건 노드와 패턴 선택 Task가 같은 거리/타겟/돌진 가능 여부를 보도록 만든다.
UCLASS()
class PROJECTR_API UBTService_PRUpdateEnemyCombatBlackboard : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_PRUpdateEnemyCombatBlackboard();

	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual FString GetStaticDescription() const override;

protected:
	// 아래 키 이름들은 Soldier_Armored Blackboard 에셋의 키와 맞아야 한다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName CurrentTargetKey = TEXT("current_target");

	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName TargetLocationKey = TEXT("target_location");

	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName HasValidTargetKey = TEXT("has_valid_target");

	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName DistanceToTargetKey = TEXT("distance_to_target");

	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName DesiredMeleeRangeKey = TEXT("desired_melee_range");

	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName ChargeRangeMinKey = TEXT("charge_range_min");

	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName ChargeRangeMaxKey = TEXT("charge_range_max");

	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName ChargePathClearKey = TEXT("charge_path_clear");

	// Blackboard에 값이 없을 때 채워 넣는 기본 근접 유지 거리다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|SoldierArmored")
	float DefaultDesiredMeleeRange = 180.0f;

	// 돌진 패턴이 유효하다고 보는 최소 거리다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|SoldierArmored")
	float DefaultChargeRangeMin = 350.0f;

	// 돌진 패턴이 유효하다고 보는 최대 거리다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|SoldierArmored")
	float DefaultChargeRangeMax = 850.0f;

	// 돌진 경로가 막혔는지 확인할 Trace 채널이다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|SoldierArmored")
	TEnumAsByte<ECollisionChannel> ChargeTraceChannel = ECC_Visibility;
};

// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_PRUpdateEnemyCombatBlackboard.generated.h"

UCLASS()
class PROJECTR_API UBTService_PRUpdateEnemyCombatBlackboard : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_PRUpdateEnemyCombatBlackboard();

	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual FString GetStaticDescription() const override;

protected:
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

	UPROPERTY(EditAnywhere, Category = "ProjectR|SoldierArmored")
	float DefaultDesiredMeleeRange = 180.0f;

	UPROPERTY(EditAnywhere, Category = "ProjectR|SoldierArmored")
	float DefaultChargeRangeMin = 350.0f;

	UPROPERTY(EditAnywhere, Category = "ProjectR|SoldierArmored")
	float DefaultChargeRangeMax = 850.0f;

	UPROPERTY(EditAnywhere, Category = "ProjectR|SoldierArmored")
	TEnumAsByte<ECollisionChannel> ChargeTraceChannel = ECC_Visibility;
};

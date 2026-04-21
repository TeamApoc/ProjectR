// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_PRSelectEnemyPattern.generated.h"

UCLASS()
class PROJECTR_API UBTTask_PRSelectEnemyPattern : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_PRSelectEnemyPattern();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;

protected:
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName CurrentTargetKey = TEXT("current_target");

	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName HasLOSKey = TEXT("has_los");

	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName TacticalModeKey = TEXT("tactical_mode");

	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName SelectedAbilityTagKey = TEXT("selected_ability_tag");
};

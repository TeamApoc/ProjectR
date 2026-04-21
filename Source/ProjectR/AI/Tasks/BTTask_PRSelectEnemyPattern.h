// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_PRSelectEnemyPattern.generated.h"

// PatternDataAsset의 조건/가중치를 평가해서 이번에 실행할 AbilityTag를 고르는 Task다.
// 결과는 Blackboard의 selected_ability_tag(Name)에 기록되고 ActivateEnemyAbility Task가 읽는다.
UCLASS()
class PROJECTR_API UBTTask_PRSelectEnemyPattern : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_PRSelectEnemyPattern();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;

protected:
	// 아래 키 이름들은 Blackboard 에셋과 맞아야 한다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName CurrentTargetKey = TEXT("current_target");

	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName HasLOSKey = TEXT("has_los");

	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName TacticalModeKey = TEXT("tactical_mode");

	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName SelectedAbilityTagKey = TEXT("selected_ability_tag");
};

// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "GameplayAbilitySpec.h"
#include "GameplayTagContainer.h"
#include "BTTask_PRActivateEnemyAbility.generated.h"

class UPRAbilitySystemComponent;

// BT에서 선택한 Gameplay Ability를 서버 ASC에 실행 요청하는 Task다.
// AbilityTag를 직접 지정하거나, Blackboard의 selected_ability_tag 값을 읽어 실행한다.
UCLASS()
class PROJECTR_API UBTTask_PRActivateEnemyAbility : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_PRActivateEnemyAbility();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual FString GetStaticDescription() const override;

protected:
	// 고정 Ability를 실행하고 싶을 때 지정한다. 비어 있으면 Blackboard 값을 사용한다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Ability", meta = (Categories = "Ability.Enemy,Ability.Boss"))
	FGameplayTag AbilityTag;

	// SelectEnemyPattern Task가 기록한 AbilityTag 이름을 읽는 키다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Ability")
	FName AbilityTagBlackboardKey = TEXT("selected_ability_tag");

	// true면 Ability가 끝날 때까지 BT 실행을 InProgress로 유지한다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Ability")
	bool bWaitUntilAbilityEnds = true;

private:
	// TickTask에서 활성 Ability가 끝났는지 확인하기 위해 캐시한다.
	UPROPERTY()
	TObjectPtr<UPRAbilitySystemComponent> ActiveAbilitySystemComponent;

	FGameplayAbilitySpecHandle ActiveAbilityHandle;
};

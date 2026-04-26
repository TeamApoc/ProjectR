// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "GameplayAbilitySpec.h"
#include "GameplayTagContainer.h"
#include "ProjectR/AI/PREnemyAITypes.h"
#include "BTTask_PRActivateEnemyAbility.generated.h"

class UBehaviorTreeComponent;
class UPRAbilitySystemComponent;
struct FAbilityEndedData;

// BT에서 선택된 Gameplay Ability를 서버 ASC에 실행 요청하는 Task다.
// 고정 AbilityTag를 쓰거나 Blackboard의 selected_ability_tag 값을 읽어 실행한다.
UCLASS()
class PROJECTR_API UBTTask_PRActivateEnemyAbility : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_PRActivateEnemyAbility();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual FString GetStaticDescription() const override;

protected:
	// 고정 Ability를 실행하고 싶을 때 지정한다. 비어 있으면 Blackboard 값을 사용한다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Ability", meta = (Categories = "Ability.Enemy,Ability.Boss"))
	FGameplayTag AbilityTag;

	// SelectEnemyPattern Task가 기록한 AbilityTag 이름을 읽는 키다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Ability")
	FName AbilityTagBlackboardKey = TEXT("selected_ability_tag");

	// 전술 상태를 저장하는 Blackboard 키다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Ability")
	FName TacticalModeKey = TEXT("tactical_mode");

	// true면 Ability가 끝날 때까지 BT 실행을 InProgress로 유지한다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Ability")
	bool bWaitUntilAbilityEnds = true;

	// Abort 요청이 와도 Ability 종료까지 BT 전환 지연
	UPROPERTY(EditAnywhere, Category = "ProjectR|Ability", meta = (EditCondition = "bWaitUntilAbilityEnds"))
	bool bDelayAbortUntilAbilityEnds = true;

	// true면 Ability가 끝난 뒤 전술 상태를 지정한 값으로 되돌린다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Ability")
	bool bSetTacticalModeAfterAbilityEnds = true;

	UPROPERTY(EditAnywhere, Category = "ProjectR|Ability", meta = (EditCondition = "bSetTacticalModeAfterAbilityEnds"))
	EPRTacticalMode TacticalModeAfterAbilityEnds = EPRTacticalMode::Chase;

private:
	void ApplyPostAbilityBlackboardUpdates(UBehaviorTreeComponent& OwnerComp);
	void BindAbilityEndDelegate(UBehaviorTreeComponent& OwnerComp, UPRAbilitySystemComponent* ASC);
	void ClearAbilityEndDelegate();
	void HandleObservedAbilityEnded(const FAbilityEndedData& EndedData);
	bool IsObservedAbilityActive() const;
	void FinishObservedAbilityWait(UBehaviorTreeComponent& OwnerComp, EBTNodeResult::Type TaskResult);

	// 대기 중인 Ability가 끝났을 때 BT를 바로 깨우기 위해 캐시한다.
	UPROPERTY()
	TObjectPtr<UPRAbilitySystemComponent> ActiveAbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UBehaviorTreeComponent> ActiveBehaviorTreeComponent;

	FGameplayAbilitySpecHandle ActiveAbilityHandle;
	FDelegateHandle AbilityEndedDelegateHandle;
	bool bAbortRequested = false;
};

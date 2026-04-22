// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "GameplayAbilitySpec.h"
#include "GameplayTagContainer.h"
#include "ProjectR/AI/PREnemyAITypes.h"
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

	// SelectEnemyPattern이 기록한 다음 콤보 단계 값을 읽는 키다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Ability")
	FName SelectedNextComboIndexKey = TEXT("selected_next_combo_index");

	// 실제 콤보 단계를 저장하는 Blackboard 키다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Ability")
	FName ComboIndexKey = TEXT("combo_index");

	// 전술 상태를 저장하는 Blackboard 키다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Ability")
	FName TacticalModeKey = TEXT("tactical_mode");

	// true면 Ability가 끝날 때까지 BT 실행을 InProgress로 유지한다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Ability")
	bool bWaitUntilAbilityEnds = true;

	// true면 Ability 실행 성공 후 selected_next_combo_index를 combo_index에 반영한다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Ability")
	bool bApplySelectedComboIndex = true;

	// true면 Ability가 끝난 뒤 전술 상태를 지정한 값으로 되돌린다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Ability")
	bool bSetTacticalModeAfterAbilityEnds = true;

	UPROPERTY(EditAnywhere, Category = "ProjectR|Ability", meta = (EditCondition = "bSetTacticalModeAfterAbilityEnds"))
	EPRTacticalMode TacticalModeAfterAbilityEnds = EPRTacticalMode::Chase;

private:
	void ApplyPostAbilityBlackboardUpdates(UBehaviorTreeComponent& OwnerComp);

	// TickTask에서 활성 Ability가 끝났는지 확인하기 위해 캐시한다.
	UPROPERTY()
	TObjectPtr<UPRAbilitySystemComponent> ActiveAbilitySystemComponent;

	FGameplayAbilitySpecHandle ActiveAbilityHandle;

	int32 PendingNextComboIndex = INDEX_NONE;
};

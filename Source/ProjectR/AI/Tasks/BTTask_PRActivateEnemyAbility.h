// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "GameplayAbilitySpec.h"
#include "GameplayTagContainer.h"
#include "BTTask_PRActivateEnemyAbility.generated.h"

class UPRAbilitySystemComponent;

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
	UPROPERTY(EditAnywhere, Category = "ProjectR|Ability", meta = (Categories = "Ability.Enemy,Ability.Boss"))
	FGameplayTag AbilityTag;

	UPROPERTY(EditAnywhere, Category = "ProjectR|Ability")
	FName AbilityTagBlackboardKey = TEXT("selected_ability_tag");

	UPROPERTY(EditAnywhere, Category = "ProjectR|Ability")
	bool bWaitUntilAbilityEnds = true;

private:
	UPROPERTY()
	TObjectPtr<UPRAbilitySystemComponent> ActiveAbilitySystemComponent;

	FGameplayAbilitySpecHandle ActiveAbilityHandle;
};

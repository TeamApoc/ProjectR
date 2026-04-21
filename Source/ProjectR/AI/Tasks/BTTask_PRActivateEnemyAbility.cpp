// Copyright ProjectR. All Rights Reserved.

#include "BTTask_PRActivateEnemyAbility.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/Character/Enemy/PREnemyInterface.h"

UBTTask_PRActivateEnemyAbility::UBTTask_PRActivateEnemyAbility()
{
	NodeName = TEXT("Activate Enemy Ability");
	bNotifyTick = true;
	bCreateNodeInstance = true;
}

EBTNodeResult::Type UBTTask_PRActivateEnemyAbility::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	ActiveAbilitySystemComponent = nullptr;
	ActiveAbilityHandle = FGameplayAbilitySpecHandle();

	FGameplayTag ResolvedAbilityTag = AbilityTag;
	if (!ResolvedAbilityTag.IsValid() && AbilityTagBlackboardKey != NAME_None)
	{
		if (UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent())
		{
			const FName AbilityTagName = BlackboardComponent->GetValueAsName(AbilityTagBlackboardKey);
			if (AbilityTagName != NAME_None)
			{
				ResolvedAbilityTag = FGameplayTag::RequestGameplayTag(AbilityTagName, false);
			}
		}
	}

	if (!ResolvedAbilityTag.IsValid())
	{
		return EBTNodeResult::Failed;
	}

	const AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* ControlledPawn = IsValid(AIController) ? AIController->GetPawn() : nullptr;
	IPREnemyInterface* EnemyInterface = Cast<IPREnemyInterface>(ControlledPawn);
	if (EnemyInterface == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	UPRAbilitySystemComponent* ASC = EnemyInterface->GetEnemyAbilitySystemComponent();
	if (!IsValid(ASC) || !ControlledPawn->HasAuthority())
	{
		return EBTNodeResult::Failed;
	}

	if (!ASC->TryActivateAbilityOnServer(ResolvedAbilityTag, ActiveAbilityHandle))
	{
		return EBTNodeResult::Failed;
	}

	if (!bWaitUntilAbilityEnds)
	{
		return EBTNodeResult::Succeeded;
	}

	ActiveAbilitySystemComponent = ASC;

	const FGameplayAbilitySpec* ActiveSpec = ASC->FindAbilitySpecFromHandle(ActiveAbilityHandle);
	return (ActiveSpec != nullptr && ActiveSpec->IsActive())
		? EBTNodeResult::InProgress
		: EBTNodeResult::Succeeded;
}

void UBTTask_PRActivateEnemyAbility::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	if (!IsValid(ActiveAbilitySystemComponent) || !ActiveAbilityHandle.IsValid())
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	const FGameplayAbilitySpec* ActiveSpec = ActiveAbilitySystemComponent->FindAbilitySpecFromHandle(ActiveAbilityHandle);
	if (ActiveSpec == nullptr || !ActiveSpec->IsActive())
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

FString UBTTask_PRActivateEnemyAbility::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s\nAbilityTag: %s\nWait: %s"),
		*Super::GetStaticDescription(),
		AbilityTag.IsValid() ? *AbilityTag.ToString() : *FString::Printf(TEXT("BB:%s"), *AbilityTagBlackboardKey.ToString()),
		bWaitUntilAbilityEnds ? TEXT("true") : TEXT("false"));
}

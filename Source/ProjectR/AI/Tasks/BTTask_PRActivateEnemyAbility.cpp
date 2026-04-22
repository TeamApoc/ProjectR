// Copyright ProjectR. All Rights Reserved.

#include "BTTask_PRActivateEnemyAbility.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/Character/Enemy/PREnemyInterface.h"

namespace
{
	bool HasBlackboardKey(const UBlackboardComponent* BlackboardComponent, const FName KeyName)
	{
		return IsValid(BlackboardComponent)
			&& KeyName != NAME_None
			&& BlackboardComponent->GetKeyID(KeyName) != FBlackboard::InvalidKey;
	}
}

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
	PendingNextComboIndex = INDEX_NONE;

	FGameplayTag ResolvedAbilityTag = AbilityTag;
	if (!ResolvedAbilityTag.IsValid() && AbilityTagBlackboardKey != NAME_None)
	{
		// 패턴 선택 Task는 Blackboard에 태그 이름(Name)만 기록한다.
		// 여기서 실제 FGameplayTag로 복원해 ASC 활성화 요청에 사용한다.
		if (UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent())
		{
			const FName AbilityTagName = BlackboardComponent->GetValueAsName(AbilityTagBlackboardKey);
			if (AbilityTagName != NAME_None)
			{
				ResolvedAbilityTag = FGameplayTag::RequestGameplayTag(AbilityTagName, false);
			}
		}
	}

	if (bApplySelectedComboIndex && !AbilityTag.IsValid())
	{
		if (UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent())
		{
			if (HasBlackboardKey(BlackboardComponent, SelectedNextComboIndexKey))
			{
				PendingNextComboIndex = BlackboardComponent->GetValueAsInt(SelectedNextComboIndexKey);
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

	// 몬스터 Ability 실행은 서버 ASC에서만 시도한다.
	if (!ASC->TryActivateAbilityOnServer(ResolvedAbilityTag, ActiveAbilityHandle))
	{
		return EBTNodeResult::Failed;
	}

	if (!bWaitUntilAbilityEnds)
	{
		ApplyPostAbilityBlackboardUpdates(OwnerComp);
		return EBTNodeResult::Succeeded;
	}

	ActiveAbilitySystemComponent = ASC;

	// Ability가 즉시 끝나는 경우도 있으므로, 활성 상태를 한 번 확인한 뒤 InProgress 여부를 결정한다.
	const FGameplayAbilitySpec* ActiveSpec = ASC->FindAbilitySpecFromHandle(ActiveAbilityHandle);
	if (ActiveSpec != nullptr && ActiveSpec->IsActive())
	{
		return EBTNodeResult::InProgress;
	}

	ApplyPostAbilityBlackboardUpdates(OwnerComp);
	return EBTNodeResult::Succeeded;
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
		ApplyPostAbilityBlackboardUpdates(OwnerComp);
		// Ability가 끝나면 BT Sequence가 다음 노드로 넘어갈 수 있게 성공 처리한다.
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

void UBTTask_PRActivateEnemyAbility::ApplyPostAbilityBlackboardUpdates(UBehaviorTreeComponent& OwnerComp)
{
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	if (!IsValid(BlackboardComponent))
	{
		return;
	}

	if (bApplySelectedComboIndex
		&& PendingNextComboIndex != INDEX_NONE
		&& HasBlackboardKey(BlackboardComponent, ComboIndexKey))
	{
		// Select Task가 DataAsset 규칙에서 읽어온 다음 콤보 단계를 실제 Blackboard 상태로 확정한다.
		BlackboardComponent->SetValueAsInt(ComboIndexKey, PendingNextComboIndex);
	}

	if (bSetTacticalModeAfterAbilityEnds && HasBlackboardKey(BlackboardComponent, TacticalModeKey))
	{
		BlackboardComponent->SetValueAsEnum(TacticalModeKey, static_cast<uint8>(TacticalModeAfterAbilityEnds));
	}
}

FString UBTTask_PRActivateEnemyAbility::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s\nAbilityTag: %s\nWait: %s\nPost Mode: %s"),
		*Super::GetStaticDescription(),
		AbilityTag.IsValid() ? *AbilityTag.ToString() : *FString::Printf(TEXT("BB:%s"), *AbilityTagBlackboardKey.ToString()),
		bWaitUntilAbilityEnds ? TEXT("true") : TEXT("false"),
		bSetTacticalModeAfterAbilityEnds
			? *StaticEnum<EPRTacticalMode>()->GetNameStringByValue(static_cast<int64>(TacticalModeAfterAbilityEnds))
			: TEXT("None"));
}

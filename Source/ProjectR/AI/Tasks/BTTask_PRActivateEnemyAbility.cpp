// Copyright ProjectR. All Rights Reserved.

#include "BTTask_PRActivateEnemyAbility.h"

#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/Character/Enemy/PREnemyInterface.h"

namespace
{
	bool HasActivateAbilityBlackboardKey(const UBlackboardComponent* BlackboardComponent, const FName KeyName)
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
	ClearAbilityEndDelegate();
	ActiveAbilityHandle = FGameplayAbilitySpecHandle();
	PendingNextComboIndex = INDEX_NONE;

	FGameplayTag ResolvedAbilityTag = AbilityTag;
	if (!ResolvedAbilityTag.IsValid() && AbilityTagBlackboardKey != NAME_None)
	{
		// 패턴 선택 Task는 Blackboard에 태그 이름만 기록하므로, 여기에서 실제 GameplayTag로 복원한다.
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
			if (HasActivateAbilityBlackboardKey(BlackboardComponent, SelectedNextComboIndexKey))
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

	BindAbilityEndDelegate(OwnerComp, ASC);

	// 즉시 종료되는 Ability는 델리게이트를 기다리지 않고 현재 Spec 상태로 바로 완료 처리한다.
	const FGameplayAbilitySpec* ActiveSpec = ASC->FindAbilitySpecFromHandle(ActiveAbilityHandle);
	if (ActiveSpec != nullptr && ActiveSpec->IsActive())
	{
		return EBTNodeResult::InProgress;
	}

	ClearAbilityEndDelegate();
	ApplyPostAbilityBlackboardUpdates(OwnerComp);
	return EBTNodeResult::Succeeded;
}

EBTNodeResult::Type UBTTask_PRActivateEnemyAbility::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	ClearAbilityEndDelegate();
	return EBTNodeResult::Aborted;
}

void UBTTask_PRActivateEnemyAbility::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	if (!IsValid(ActiveAbilitySystemComponent) || !ActiveAbilityHandle.IsValid())
	{
		ClearAbilityEndDelegate();
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// 델리게이트가 정상 호출되지 않는 예외 상황을 대비한 안전망이다.
	const FGameplayAbilitySpec* ActiveSpec = ActiveAbilitySystemComponent->FindAbilitySpecFromHandle(ActiveAbilityHandle);
	if (ActiveSpec == nullptr || !ActiveSpec->IsActive())
	{
		ClearAbilityEndDelegate();
		ApplyPostAbilityBlackboardUpdates(OwnerComp);
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
		&& HasActivateAbilityBlackboardKey(BlackboardComponent, ComboIndexKey))
	{
		// Select Task가 DataAsset 규칙에서 읽은 다음 콤보 단계를 실제 Blackboard 상태로 확정한다.
		BlackboardComponent->SetValueAsInt(ComboIndexKey, PendingNextComboIndex);
	}

	if (bSetTacticalModeAfterAbilityEnds && HasActivateAbilityBlackboardKey(BlackboardComponent, TacticalModeKey))
	{
		BlackboardComponent->SetValueAsEnum(TacticalModeKey, static_cast<uint8>(TacticalModeAfterAbilityEnds));
	}
}

void UBTTask_PRActivateEnemyAbility::BindAbilityEndDelegate(UBehaviorTreeComponent& OwnerComp, UPRAbilitySystemComponent* ASC)
{
	if (!IsValid(ASC))
	{
		return;
	}

	ActiveAbilitySystemComponent = ASC;
	ActiveBehaviorTreeComponent = &OwnerComp;

	UAbilitySystemComponent* BaseASC = ASC;
	AbilityEndedDelegateHandle = BaseASC->OnAbilityEnded.AddUObject(this, &UBTTask_PRActivateEnemyAbility::HandleObservedAbilityEnded);
}

void UBTTask_PRActivateEnemyAbility::ClearAbilityEndDelegate()
{
	if (AbilityEndedDelegateHandle.IsValid())
	{
		UAbilitySystemComponent* BaseASC = ActiveAbilitySystemComponent.Get();
		if (IsValid(BaseASC))
		{
			BaseASC->OnAbilityEnded.Remove(AbilityEndedDelegateHandle);
		}

		AbilityEndedDelegateHandle = FDelegateHandle();
	}

	ActiveAbilitySystemComponent = nullptr;
	ActiveBehaviorTreeComponent = nullptr;
	ActiveAbilityHandle = FGameplayAbilitySpecHandle();
}

void UBTTask_PRActivateEnemyAbility::HandleObservedAbilityEnded(const FAbilityEndedData& EndedData)
{
	if (!ActiveAbilityHandle.IsValid() || EndedData.AbilitySpecHandle != ActiveAbilityHandle)
	{
		return;
	}

	UBehaviorTreeComponent* OwnerComp = ActiveBehaviorTreeComponent.Get();
	if (!IsValid(OwnerComp))
	{
		ClearAbilityEndDelegate();
		return;
	}

	ClearAbilityEndDelegate();
	ApplyPostAbilityBlackboardUpdates(*OwnerComp);
	FinishLatentTask(*OwnerComp, EBTNodeResult::Succeeded);
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

// Copyright ProjectR. All Rights Reserved.

#include "BTTask_PRSelectPenitentRangedAbility.h"

#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/Character/Enemy/PREnemyInterface.h"
#include "ProjectR/PRGameplayTags.h"

namespace
{
	bool HasPenitentRangedBlackboardKey(const UBlackboardComponent* BlackboardComponent, const FName KeyName)
	{
		return IsValid(BlackboardComponent)
			&& KeyName != NAME_None
			&& BlackboardComponent->GetKeyID(KeyName) != FBlackboard::InvalidKey;
	}

	UAbilitySystemComponent* ResolvePenitentAbilitySystem(const UBehaviorTreeComponent& OwnerComp)
	{
		const AAIController* AIController = OwnerComp.GetAIOwner();
		APawn* ControlledPawn = IsValid(AIController) ? AIController->GetPawn() : nullptr;
		IPREnemyInterface* EnemyInterface = Cast<IPREnemyInterface>(ControlledPawn);
		return EnemyInterface ? EnemyInterface->GetEnemyAbilitySystemComponent() : nullptr;
	}
}

UBTTask_PRSelectPenitentRangedAbility::UBTTask_PRSelectPenitentRangedAbility()
{
	NodeName = TEXT("Select Penitent Ranged Ability");
}

EBTNodeResult::Type UBTTask_PRSelectPenitentRangedAbility::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	if (!HasPenitentRangedBlackboardKey(BlackboardComponent, SelectedAbilityTagKey))
	{
		return EBTNodeResult::Failed;
	}

	const UAbilitySystemComponent* AbilitySystemComponent = ResolvePenitentAbilitySystem(OwnerComp);
	if (!IsValid(AbilitySystemComponent))
	{
		return EBTNodeResult::Failed;
	}

	TArray<FGameplayTag> CandidateTags;
	CandidateTags.Add(PRGameplayTags::Ability_Enemy_Penitent_Projectile);

	const bool bHasBarrier = AbilitySystemComponent->HasMatchingGameplayTag(PRGameplayTags::State_Enemy_Penitent_BarrierSummon);
	CandidateTags.Add(bHasBarrier
		? PRGameplayTags::Ability_Enemy_Penitent_BarrierFire
		: PRGameplayTags::Ability_Enemy_Penitent_BarrierSummon);

	FGameplayTag LastUsedRangedAbility;
	if (HasPenitentRangedBlackboardKey(BlackboardComponent, LastUsedRangedAbilityKey))
	{
		const FName LastUsedAbilityName = BlackboardComponent->GetValueAsName(LastUsedRangedAbilityKey);
		if (LastUsedAbilityName != NAME_None)
		{
			LastUsedRangedAbility = FGameplayTag::RequestGameplayTag(LastUsedAbilityName, false);
		}
	}

	FGameplayTag SelectedAbilityTag = CandidateTags[0];
	if (LastUsedRangedAbility.IsValid() && SelectedAbilityTag == LastUsedRangedAbility && CandidateTags.Num() > 1)
	{
		// 직전 원거리 Ability 후순위
		SelectedAbilityTag = CandidateTags[1];
	}

	if (!SelectedAbilityTag.IsValid())
	{
		return EBTNodeResult::Failed;
	}

	BlackboardComponent->SetValueAsName(SelectedAbilityTagKey, SelectedAbilityTag.GetTagName());
	return EBTNodeResult::Succeeded;
}

FString UBTTask_PRSelectPenitentRangedAbility::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s\nLast Key: %s\nWrite Key: %s"),
		*Super::GetStaticDescription(),
		*LastUsedRangedAbilityKey.ToString(),
		*SelectedAbilityTagKey.ToString());
}

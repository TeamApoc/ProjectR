// Copyright ProjectR. All Rights Reserved.

#include "BTService_PRUpdateEnemyCombatBlackboard.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"

namespace
{
	bool HasBlackboardKey(const UBlackboardComponent* BlackboardComponent, const FName KeyName)
	{
		return IsValid(BlackboardComponent)
			&& KeyName != NAME_None
			&& BlackboardComponent->GetKeyID(KeyName) != FBlackboard::InvalidKey;
	}
}

UBTService_PRUpdateEnemyCombatBlackboard::UBTService_PRUpdateEnemyCombatBlackboard()
{
	NodeName = TEXT("Update Enemy Combat Blackboard");
	Interval = 0.2f;
	RandomDeviation = 0.05f;
}

void UBTService_PRUpdateEnemyCombatBlackboard::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	const AAIController* AIController = OwnerComp.GetAIOwner();
	const APawn* ControlledPawn = IsValid(AIController) ? AIController->GetPawn() : nullptr;
	if (!IsValid(BlackboardComponent) || !IsValid(ControlledPawn))
	{
		return;
	}

	AActor* CurrentTarget = Cast<AActor>(BlackboardComponent->GetValueAsObject(CurrentTargetKey));
	const bool bHasValidTarget = IsValid(CurrentTarget);
	BlackboardComponent->SetValueAsBool(HasValidTargetKey, bHasValidTarget);

	if (!bHasValidTarget)
	{
		// 타겟이 없을 때 이전 거리/돌진 가능 값이 남아 BT 조건을 오염시키지 않도록 초기화한다.
		BlackboardComponent->SetValueAsFloat(DistanceToTargetKey, 0.0f);
		BlackboardComponent->SetValueAsBool(ChargePathClearKey, false);
		if (bResetComboWhenTargetInvalid && HasBlackboardKey(BlackboardComponent, ComboIndexKey))
		{
			BlackboardComponent->SetValueAsInt(ComboIndexKey, 0);
		}
		return;
	}

	const FVector OwnerLocation = ControlledPawn->GetActorLocation();
	const FVector TargetLocation = CurrentTarget->GetActorLocation();
	const float DistanceToTarget = FVector::Dist(OwnerLocation, TargetLocation);

	BlackboardComponent->SetValueAsVector(TargetLocationKey, TargetLocation);
	BlackboardComponent->SetValueAsFloat(DistanceToTargetKey, DistanceToTarget);

	if (BlackboardComponent->GetValueAsFloat(DesiredMeleeRangeKey) <= 0.0f)
	{
		// 에디터에서 Blackboard 기본값을 비워둬도 BT가 최소한의 거리 기준을 갖도록 보정한다.
		BlackboardComponent->SetValueAsFloat(DesiredMeleeRangeKey, DefaultDesiredMeleeRange);
	}

	if (BlackboardComponent->GetValueAsFloat(ChargeRangeMinKey) <= 0.0f)
	{
		BlackboardComponent->SetValueAsFloat(ChargeRangeMinKey, DefaultChargeRangeMin);
	}

	if (BlackboardComponent->GetValueAsFloat(ChargeRangeMaxKey) <= 0.0f)
	{
		BlackboardComponent->SetValueAsFloat(ChargeRangeMaxKey, DefaultChargeRangeMax);
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PREnemyChargePath), false, ControlledPawn);
	QueryParams.AddIgnoredActor(ControlledPawn);

	FHitResult HitResult;
	const bool bBlocked = ControlledPawn->GetWorld()->LineTraceSingleByChannel(
		HitResult,
		OwnerLocation,
		TargetLocation,
		ChargeTraceChannel,
		QueryParams);

	const bool bChargeRange = DistanceToTarget >= BlackboardComponent->GetValueAsFloat(ChargeRangeMinKey)
		&& DistanceToTarget <= BlackboardComponent->GetValueAsFloat(ChargeRangeMaxKey);
	// 돌진은 거리 조건을 만족하고, 타겟까지의 직선 경로가 막히지 않았을 때만 허용한다.
	const bool bChargePathClear = bChargeRange && (!bBlocked || HitResult.GetActor() == CurrentTarget);
	BlackboardComponent->SetValueAsBool(ChargePathClearKey, bChargePathClear);

	if (bResetComboWhenOutsideResetDistance
		&& HasBlackboardKey(BlackboardComponent, ComboIndexKey)
		&& HasBlackboardKey(BlackboardComponent, ComboResetDistanceKey))
	{
		const float ComboResetDistance = BlackboardComponent->GetValueAsFloat(ComboResetDistanceKey);
		if (ComboResetDistance > 0.0f && DistanceToTarget > ComboResetDistance)
		{
			// 콤보 유지 거리는 Blackboard 값으로만 판단한다. 값이 없으면 거리 기반 초기화는 하지 않는다.
			BlackboardComponent->SetValueAsInt(ComboIndexKey, 0);
		}
	}
}

FString UBTService_PRUpdateEnemyCombatBlackboard::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s\nTarget Key: %s\nDistance Key: %s"),
		*Super::GetStaticDescription(),
		*CurrentTargetKey.ToString(),
		*DistanceToTargetKey.ToString());
}

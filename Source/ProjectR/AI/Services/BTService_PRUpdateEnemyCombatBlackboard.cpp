// Copyright ProjectR. All Rights Reserved.

#include "BTService_PRUpdateEnemyCombatBlackboard.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

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
		BlackboardComponent->SetValueAsFloat(DistanceToTargetKey, 0.0f);
		BlackboardComponent->SetValueAsBool(ChargePathClearKey, false);
		return;
	}

	const FVector OwnerLocation = ControlledPawn->GetActorLocation();
	const FVector TargetLocation = CurrentTarget->GetActorLocation();
	const float DistanceToTarget = FVector::Dist(OwnerLocation, TargetLocation);

	BlackboardComponent->SetValueAsVector(TargetLocationKey, TargetLocation);
	BlackboardComponent->SetValueAsFloat(DistanceToTargetKey, DistanceToTarget);

	if (BlackboardComponent->GetValueAsFloat(DesiredMeleeRangeKey) <= 0.0f)
	{
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
	const bool bChargePathClear = bChargeRange && (!bBlocked || HitResult.GetActor() == CurrentTarget);
	BlackboardComponent->SetValueAsBool(ChargePathClearKey, bChargePathClear);
}

FString UBTService_PRUpdateEnemyCombatBlackboard::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s\nTarget Key: %s\nDistance Key: %s"),
		*Super::GetStaticDescription(),
		*CurrentTargetKey.ToString(),
		*DistanceToTargetKey.ToString());
}

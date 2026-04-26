// Copyright ProjectR. All Rights Reserved.

#include "BTService_PRUpdateEnemyCombatBlackboard.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

// ===== 초기화 =====

UBTService_PRUpdateEnemyCombatBlackboard::UBTService_PRUpdateEnemyCombatBlackboard()
{
	NodeName = TEXT("Update Enemy Combat Blackboard");
	Interval = 0.2f;
	RandomDeviation = 0.05f;
}

// ===== Blackboard 갱신 =====

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
		return;
	}

	const FVector OwnerLocation = ControlledPawn->GetActorLocation();
	const FVector TargetLocation = CurrentTarget->GetActorLocation();
	const float DistanceToTarget = FVector::Dist(OwnerLocation, TargetLocation);

	BlackboardComponent->SetValueAsVector(TargetLocationKey, TargetLocation);
	BlackboardComponent->SetValueAsFloat(DistanceToTargetKey, DistanceToTarget);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PREnemyChargePath), false, ControlledPawn);
	QueryParams.AddIgnoredActor(ControlledPawn);

	FHitResult HitResult;
	const bool bBlocked = ControlledPawn->GetWorld()->LineTraceSingleByChannel(
		HitResult,
		OwnerLocation,
		TargetLocation,
		ChargeTraceChannel,
		QueryParams);

	// Sprint 거리 조건: BT Decorator / PatternDataAsset 책임
	// 경로 차단 여부만 Blackboard에 기록
	const bool bChargePathClear = !bBlocked || HitResult.GetActor() == CurrentTarget;
	BlackboardComponent->SetValueAsBool(ChargePathClearKey, bChargePathClear);
}

// ===== 에디터 표시 =====

FString UBTService_PRUpdateEnemyCombatBlackboard::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s\nTarget Key: %s\nDistance Key: %s"),
		*Super::GetStaticDescription(),
		*CurrentTargetKey.ToString(),
		*DistanceToTargetKey.ToString());
}

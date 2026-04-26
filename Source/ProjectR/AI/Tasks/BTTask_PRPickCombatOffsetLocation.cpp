// Copyright ProjectR. All Rights Reserved.

#include "BTTask_PRPickCombatOffsetLocation.h"

#include "Algo/Reverse.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "NavigationSystem.h"
#include "ProjectR/AI/Data/PRSoldierArmoredCombatDataAsset.h"

namespace
{
	bool HasCombatOffsetBlackboardKey(const UBlackboardComponent* BlackboardComponent, const FName KeyName)
	{
		return IsValid(BlackboardComponent)
			&& KeyName != NAME_None
			&& BlackboardComponent->GetKeyID(KeyName) != FBlackboard::InvalidKey;
	}

	const FPRSoldierArmoredOffsetMoveConfig* GetOffsetMoveConfig(
		const UPRSoldierArmoredCombatDataAsset* CombatDataAsset,
		EPRCombatOffsetSelectionMode SelectionMode)
	{
		if (!IsValid(CombatDataAsset))
		{
			return nullptr;
		}

		switch (SelectionMode)
		{
		case EPRCombatOffsetSelectionMode::Reposition:
			return &CombatDataAsset->CombatRepositionConfig;
		case EPRCombatOffsetSelectionMode::Strafe:
		default:
			return &CombatDataAsset->CombatStrafeConfig;
		}
	}

	bool TryBuildProjectedOffsetLocation(
		const APawn* ControlledPawn,
		const AActor* CurrentTarget,
		const FPRSoldierArmoredOffsetMoveConfig& Config,
		const float SideSign,
		FVector& OutLocation)
	{
		if (!IsValid(ControlledPawn) || !IsValid(CurrentTarget))
		{
			return false;
		}

		FVector BaseDirection = ControlledPawn->GetActorLocation() - CurrentTarget->GetActorLocation();
		BaseDirection.Z = 0.0f;
		if (!BaseDirection.Normalize())
		{
			BaseDirection = CurrentTarget->GetActorForwardVector() * -1.0f;
			BaseDirection.Z = 0.0f;
			if (!BaseDirection.Normalize())
			{
				BaseDirection = FVector::BackwardVector;
			}
		}

		const float OffsetAngleDegrees = FMath::FRandRange(Config.MinOffsetAngleDegrees, Config.MaxOffsetAngleDegrees);
		const float OffsetDistance = FMath::FRandRange(Config.MinOffsetDistance, Config.MaxOffsetDistance);
		const FVector OffsetDirection = BaseDirection.RotateAngleAxis(SideSign * OffsetAngleDegrees, FVector::UpVector);
		const FVector RawGoalLocation = CurrentTarget->GetActorLocation() + (OffsetDirection * OffsetDistance);

		UNavigationSystemV1* NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(ControlledPawn->GetWorld());
		if (!IsValid(NavigationSystem))
		{
			OutLocation = RawGoalLocation;
			return true;
		}

		FNavLocation ProjectedLocation;
		const bool bProjected = NavigationSystem->ProjectPointToNavigation(
			RawGoalLocation,
			ProjectedLocation,
			Config.NavProjectionExtent);

		if (!bProjected)
		{
			return false;
		}

		OutLocation = ProjectedLocation.Location;
		return true;
	}
}

UBTTask_PRPickCombatOffsetLocation::UBTTask_PRPickCombatOffsetLocation()
{
	NodeName = TEXT("Pick Combat Offset Location");
}

EBTNodeResult::Type UBTTask_PRPickCombatOffsetLocation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	const AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* ControlledPawn = IsValid(AIController) ? AIController->GetPawn() : nullptr;
	if (!IsValid(BlackboardComponent)
		|| !IsValid(ControlledPawn)
		|| !HasCombatOffsetBlackboardKey(BlackboardComponent, CurrentTargetKey)
		|| !HasCombatOffsetBlackboardKey(BlackboardComponent, MoveGoalLocationKey))
	{
		return EBTNodeResult::Failed;
	}

	const AActor* CurrentTarget = Cast<AActor>(BlackboardComponent->GetValueAsObject(CurrentTargetKey));
	const FPRSoldierArmoredOffsetMoveConfig* MoveConfig = GetOffsetMoveConfig(CombatDataAsset, SelectionMode);
	if (!IsValid(CurrentTarget) || MoveConfig == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	TArray<float> SideOrder;
	if (MoveConfig->bAllowLeft)
	{
		SideOrder.Add(-1.0f);
	}
	if (MoveConfig->bAllowRight)
	{
		SideOrder.Add(1.0f);
	}

	if (SideOrder.IsEmpty())
	{
		return EBTNodeResult::Failed;
	}

	if (bRandomizeSideOrder && SideOrder.Num() > 1 && FMath::RandBool())
	{
		Algo::Reverse(SideOrder);
	}

	for (const float SideSign : SideOrder)
	{
		for (int32 AttemptIndex = 0; AttemptIndex < AttemptsPerSide; ++AttemptIndex)
		{
			FVector MoveGoalLocation = FVector::ZeroVector;
			if (!TryBuildProjectedOffsetLocation(ControlledPawn, CurrentTarget, *MoveConfig, SideSign, MoveGoalLocation))
			{
				continue;
			}

			BlackboardComponent->SetValueAsVector(MoveGoalLocationKey, MoveGoalLocation);

			if (bSetTacticalModeOnSuccess && HasCombatOffsetBlackboardKey(BlackboardComponent, TacticalModeKey))
			{
				BlackboardComponent->SetValueAsEnum(TacticalModeKey, static_cast<uint8>(TacticalModeOnSuccess));
			}

			return EBTNodeResult::Succeeded;
		}
	}

	return EBTNodeResult::Failed;
}

FString UBTTask_PRPickCombatOffsetLocation::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s\nMode: %s\nWrite Key: %s"),
		*Super::GetStaticDescription(),
		*StaticEnum<EPRCombatOffsetSelectionMode>()->GetNameStringByValue(static_cast<int64>(SelectionMode)),
		*MoveGoalLocationKey.ToString());
}

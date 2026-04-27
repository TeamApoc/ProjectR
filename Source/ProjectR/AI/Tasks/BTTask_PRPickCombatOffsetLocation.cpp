// Copyright ProjectR. All Rights Reserved.

#include "BTTask_PRPickCombatOffsetLocation.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "ProjectR/AI/Controllers/PREnemyAIController.h"
#include "ProjectR/AI/Data/PREnemyCombatDataAsset.h"
#include "ProjectR/Character/Enemy/PREnemyInterface.h"

namespace
{
	bool HasMoveGoalBlackboardKey(const UBlackboardComponent* BlackboardComponent, const FName KeyName)
	{
		return IsValid(BlackboardComponent)
			&& KeyName != NAME_None
			&& BlackboardComponent->GetKeyID(KeyName) != FBlackboard::InvalidKey;
	}

	const FPREnemyMoveQueryConfig* GetMoveQueryConfig(
		const UPREnemyCombatDataAsset* CombatDataAsset,
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

	const UPREnemyCombatDataAsset* ResolveCombatDataAsset(const APawn* ControlledPawn, const UPREnemyCombatDataAsset* OverrideAsset)
	{
		if (IsValid(OverrideAsset))
		{
			return OverrideAsset;
		}

		const IPREnemyInterface* EnemyInterface = Cast<IPREnemyInterface>(ControlledPawn);
		if (EnemyInterface == nullptr)
		{
			return nullptr;
		}

		return EnemyInterface->GetCombatDataAsset();
	}

	bool RunMoveGoalEQS(APawn* ControlledPawn, const FPREnemyMoveQueryConfig& Config, FVector& OutLocation)
	{
		if (!IsValid(ControlledPawn) || !IsValid(Config.QueryTemplate))
		{
			return false;
		}

		UWorld* World = ControlledPawn->GetWorld();
		if (!IsValid(World))
		{
			return false;
		}

		UEnvQueryManager* QueryManager = UEnvQueryManager::GetCurrent(World);
		if (!IsValid(QueryManager))
		{
			return false;
		}

		const FEnvQueryRequest QueryRequest(Config.QueryTemplate, ControlledPawn);
		const TSharedPtr<FEnvQueryResult> QueryResult = QueryManager->RunInstantQuery(QueryRequest, Config.QueryRunMode);
		if (!QueryResult.IsValid() || !QueryResult->IsSuccessful() || QueryResult->Items.Num() <= 0)
		{
			return false;
		}

		OutLocation = QueryResult->GetItemAsLocation(0);
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
	AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* ControlledPawn = IsValid(AIController) ? AIController->GetPawn() : nullptr;
	if (!IsValid(BlackboardComponent)
		|| !IsValid(ControlledPawn)
		|| !HasMoveGoalBlackboardKey(BlackboardComponent, CurrentTargetKey)
		|| !HasMoveGoalBlackboardKey(BlackboardComponent, MoveGoalLocationKey))
	{
		return EBTNodeResult::Failed;
	}

	AActor* CurrentTarget = Cast<AActor>(BlackboardComponent->GetValueAsObject(CurrentTargetKey));
	if (!IsValid(CurrentTarget))
	{
		return EBTNodeResult::Failed;
	}

	const UPREnemyCombatDataAsset* ResolvedCombatDataAsset = ResolveCombatDataAsset(ControlledPawn, CombatDataAsset);
	const FPREnemyMoveQueryConfig* MoveQueryConfig = GetMoveQueryConfig(ResolvedCombatDataAsset, SelectionMode);
	if (MoveQueryConfig == nullptr || !IsValid(MoveQueryConfig->QueryTemplate))
	{
		return EBTNodeResult::Failed;
	}

	FVector MoveGoalLocation = FVector::ZeroVector;
	if (!RunMoveGoalEQS(ControlledPawn, *MoveQueryConfig, MoveGoalLocation))
	{
		return EBTNodeResult::Failed;
	}

	BlackboardComponent->SetValueAsVector(MoveGoalLocationKey, MoveGoalLocation);

	if (APREnemyAIController* EnemyAIController = Cast<APREnemyAIController>(AIController))
	{
		EnemyAIController->ApplyCombatMovePresentationContext(
			CurrentTarget,
			MoveQueryConfig->bMaintainTargetFocus,
			MoveQueryConfig->bUseCombatAimOffset);
	}

	if (bSetTacticalModeOnSuccess && HasMoveGoalBlackboardKey(BlackboardComponent, TacticalModeKey))
	{
		BlackboardComponent->SetValueAsEnum(TacticalModeKey, static_cast<uint8>(TacticalModeOnSuccess));
	}

	return EBTNodeResult::Succeeded;
}

FString UBTTask_PRPickCombatOffsetLocation::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s\nMode: %s\nWrite Key: %s"),
		*Super::GetStaticDescription(),
		*StaticEnum<EPRCombatOffsetSelectionMode>()->GetNameStringByValue(static_cast<int64>(SelectionMode)),
		*MoveGoalLocationKey.ToString());
}

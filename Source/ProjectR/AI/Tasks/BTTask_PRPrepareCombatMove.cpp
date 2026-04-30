// Copyright ProjectR. All Rights Reserved.

#include "BTTask_PRPrepareCombatMove.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "ProjectR/AI/PREnemyAIDebug.h"
#include "ProjectR/AI/Controllers/PREnemyAIController.h"
#include "ProjectR/AI/Data/PREnemyCombatDataAsset.h"
#include "ProjectR/Character/Enemy/PREnemyInterface.h"

namespace
{
	struct FPRMoveQueryCandidate
	{
		int32 ItemIndex = INDEX_NONE;
		float Score = 0.0f;
	};

	bool HasMoveGoalBlackboardKey(const UBlackboardComponent* BlackboardComponent, const FName KeyName)
	{
		return IsValid(BlackboardComponent)
			&& KeyName != NAME_None
			&& BlackboardComponent->GetKeyID(KeyName) != FBlackboard::InvalidKey;
	}

	const FPREnemyMoveQueryConfig* GetMoveQueryConfig(
		const UPREnemyCombatDataAsset* CombatDataAsset,
		EPRCombatMoveQueryType SelectionMode)
	{
		if (!IsValid(CombatDataAsset))
		{
			return nullptr;
		}

		switch (SelectionMode)
		{
		case EPRCombatMoveQueryType::ApproachMeleeRange:
			return &CombatDataAsset->ApproachMeleeRangeConfig;
		case EPRCombatMoveQueryType::FastApproach:
			return &CombatDataAsset->FastApproachConfig;
		case EPRCombatMoveQueryType::Strafe:
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

	EEnvQueryRunMode::Type ResolveQueryRunMode(const FPREnemyMoveQueryConfig& Config)
	{
		if (Config.CandidateSelectionMode == EPREnemyQueryCandidateSelectionMode::BestScore)
		{
			return Config.QueryRunMode;
		}

		return EEnvQueryRunMode::AllMatching;
	}

	void BuildSortedCandidates(const FEnvQueryResult& QueryResult, TArray<FPRMoveQueryCandidate>& OutCandidates)
	{
		for (int32 ItemIndex = 0; ItemIndex < QueryResult.Items.Num(); ++ItemIndex)
		{
			if (!QueryResult.Items[ItemIndex].IsValid())
			{
				continue;
			}

			FPRMoveQueryCandidate Candidate;
			Candidate.ItemIndex = ItemIndex;
			Candidate.Score = QueryResult.GetItemScore(ItemIndex);
			OutCandidates.Add(Candidate);
		}

		OutCandidates.Sort([](const FPRMoveQueryCandidate& Left, const FPRMoveQueryCandidate& Right)
		{
			return Left.Score > Right.Score;
		});
	}

	void FilterTopCandidates(const FPREnemyMoveQueryConfig& Config, TArray<FPRMoveQueryCandidate>& Candidates)
	{
		if (Candidates.IsEmpty())
		{
			return;
		}

		const float MaxScore = Candidates[0].Score;
		const float MinAllowedScore = MaxScore * FMath::Clamp(Config.TopScoreCandidateRatio, 0.0f, 1.0f);
		Candidates.RemoveAll([MinAllowedScore](const FPRMoveQueryCandidate& Candidate)
		{
			return Candidate.Score < MinAllowedScore;
		});

		if (Config.TopCandidateCount > 0 && Candidates.Num() > Config.TopCandidateCount)
		{
			Candidates.SetNum(Config.TopCandidateCount);
		}
	}

	bool SelectCandidateIndex(const FPREnemyMoveQueryConfig& Config, const TArray<FPRMoveQueryCandidate>& Candidates, int32& OutItemIndex)
	{
		if (Candidates.IsEmpty())
		{
			return false;
		}

		switch (Config.CandidateSelectionMode)
		{
		case EPREnemyQueryCandidateSelectionMode::RandomTopCandidates:
		{
			const int32 PickIndex = FMath::RandRange(0, Candidates.Num() - 1);
			OutItemIndex = Candidates[PickIndex].ItemIndex;
			return true;
		}
		case EPREnemyQueryCandidateSelectionMode::WeightedRandomTopCandidates:
		{
			float TotalWeight = 0.0f;
			for (const FPRMoveQueryCandidate& Candidate : Candidates)
			{
				TotalWeight += FMath::Max(Candidate.Score, KINDA_SMALL_NUMBER);
			}

			if (TotalWeight <= 0.0f)
			{
				OutItemIndex = Candidates[0].ItemIndex;
				return true;
			}

			const float PickWeight = FMath::FRandRange(0.0f, TotalWeight);
			float AccumulatedWeight = 0.0f;
			for (const FPRMoveQueryCandidate& Candidate : Candidates)
			{
				AccumulatedWeight += FMath::Max(Candidate.Score, KINDA_SMALL_NUMBER);
				if (PickWeight <= AccumulatedWeight)
				{
					OutItemIndex = Candidate.ItemIndex;
					return true;
				}
			}

			OutItemIndex = Candidates.Last().ItemIndex;
			return true;
		}
		case EPREnemyQueryCandidateSelectionMode::BestScore:
		default:
			OutItemIndex = Candidates[0].ItemIndex;
			return true;
		}
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

		FEnvQueryRequest QueryRequest(Config.QueryTemplate, ControlledPawn);
		for (const FPREnemyEQSFloatParam& FloatParam : Config.FloatParams)
		{
			if (FloatParam.ParamName == NAME_None)
			{
				continue;
			}

			QueryRequest.SetFloatParam(FloatParam.ParamName, FloatParam.Value);
		}

		const TSharedPtr<FEnvQueryResult> QueryResult = QueryManager->RunInstantQuery(QueryRequest, ResolveQueryRunMode(Config));
		if (!QueryResult.IsValid() || !QueryResult->IsSuccessful() || QueryResult->Items.Num() <= 0)
		{
			return false;
		}

		TArray<FPRMoveQueryCandidate> Candidates;
		BuildSortedCandidates(*QueryResult, Candidates);
		FilterTopCandidates(Config, Candidates);

		int32 SelectedItemIndex = INDEX_NONE;
		if (!SelectCandidateIndex(Config, Candidates, SelectedItemIndex) || SelectedItemIndex == INDEX_NONE)
		{
			return false;
		}

		OutLocation = QueryResult->GetItemAsLocation(SelectedItemIndex);
		return true;
	}
}

UBTTask_PRPrepareCombatMove::UBTTask_PRPrepareCombatMove()
{
	NodeName = TEXT("Prepare Combat Move");
}

EBTNodeResult::Type UBTTask_PRPrepareCombatMove::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
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
		if (PREnemyAIDebug::IsPatternLogEnabled())
		{
			UE_LOG(
				LogPREnemyAI,
				Warning,
				TEXT("[CombatMove] Query failed Mode=%s Pawn=%s"),
				*StaticEnum<EPRCombatMoveQueryType>()->GetNameStringByValue(static_cast<int64>(SelectionMode)),
				*GetNameSafe(ControlledPawn));
		}
		return EBTNodeResult::Failed;
	}

	BlackboardComponent->SetValueAsVector(MoveGoalLocationKey, MoveGoalLocation);

	if (bSetTacticalModeOnSuccess && HasMoveGoalBlackboardKey(BlackboardComponent, TacticalModeKey))
	{
		if (APREnemyAIController* EnemyAIController = Cast<APREnemyAIController>(AIController))
		{
			EnemyAIController->ApplyTacticalModeState(
				TacticalModeOnSuccess,
				CurrentTarget,
				&MoveQueryConfig->PresentationConfig);
		}
		else
		{
			BlackboardComponent->SetValueAsEnum(TacticalModeKey, static_cast<uint8>(TacticalModeOnSuccess));
		}
	}
	else if (APREnemyAIController* EnemyAIController = Cast<APREnemyAIController>(AIController))
	{
		EnemyAIController->ApplyCombatMovePresentationContext(CurrentTarget, MoveQueryConfig->PresentationConfig);
	}

	if (PREnemyAIDebug::IsPatternLogEnabled())
	{
		UE_LOG(
			LogPREnemyAI,
			Verbose,
			TEXT("[CombatMove] Success Mode=%s Goal=%s TacticalModeWrite=%s Pawn=%s"),
			*StaticEnum<EPRCombatMoveQueryType>()->GetNameStringByValue(static_cast<int64>(SelectionMode)),
			*MoveGoalLocation.ToCompactString(),
			bSetTacticalModeOnSuccess
				? *StaticEnum<EPRTacticalMode>()->GetNameStringByValue(static_cast<int64>(TacticalModeOnSuccess))
				: TEXT("None"),
			*GetNameSafe(ControlledPawn));
	}

	return EBTNodeResult::Succeeded;
}

FString UBTTask_PRPrepareCombatMove::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s\nMode: %s\nWrite Key: %s"),
		*Super::GetStaticDescription(),
		*StaticEnum<EPRCombatMoveQueryType>()->GetNameStringByValue(static_cast<int64>(SelectionMode)),
		*MoveGoalLocationKey.ToString());
}

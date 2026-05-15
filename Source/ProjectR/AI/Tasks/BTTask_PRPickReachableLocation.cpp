// Copyright ProjectR. All Rights Reserved.

#include "BTTask_PRPickReachableLocation.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "NavFilters/NavigationQueryFilter.h"
#include "NavigationData.h"
#include "NavigationSystem.h"

namespace
{
	struct FPRLocationQueryCandidate
	{
		int32 ItemIndex = INDEX_NONE;
		float Score = 0.0f;
	};

	EEnvQueryRunMode::Type ResolveQueryRunMode(const FPREnemyLocationQueryConfig& Config)
	{
		if (Config.CandidateSelectionMode == EPREnemyQueryCandidateSelectionMode::BestScore)
		{
			return Config.QueryRunMode;
		}

		return EEnvQueryRunMode::AllMatching;
	}

	void BuildSortedCandidates(const FEnvQueryResult& QueryResult, TArray<FPRLocationQueryCandidate>& OutCandidates)
	{
		for (int32 ItemIndex = 0; ItemIndex < QueryResult.Items.Num(); ++ItemIndex)
		{
			if (!QueryResult.Items[ItemIndex].IsValid())
			{
				continue;
			}

			FPRLocationQueryCandidate Candidate;
			Candidate.ItemIndex = ItemIndex;
			Candidate.Score = QueryResult.GetItemScore(ItemIndex);
			OutCandidates.Add(Candidate);
		}

		OutCandidates.Sort([](const FPRLocationQueryCandidate& Left, const FPRLocationQueryCandidate& Right)
		{
			return Left.Score > Right.Score;
		});
	}

	void FilterTopCandidates(const FPREnemyLocationQueryConfig& Config, TArray<FPRLocationQueryCandidate>& Candidates)
	{
		if (Candidates.IsEmpty())
		{
			return;
		}

		const float MaxScore = Candidates[0].Score;
		const float MinAllowedScore = MaxScore * FMath::Clamp(Config.TopScoreCandidateRatio, 0.0f, 1.0f);
		Candidates.RemoveAll([MinAllowedScore](const FPRLocationQueryCandidate& Candidate)
		{
			return Candidate.Score < MinAllowedScore;
		});

		if (Config.TopCandidateCount > 0 && Candidates.Num() > Config.TopCandidateCount)
		{
			Candidates.SetNum(Config.TopCandidateCount);
		}
	}

	bool SelectCandidateIndex(const FPREnemyLocationQueryConfig& Config, const TArray<FPRLocationQueryCandidate>& Candidates, int32& OutItemIndex)
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
			for (const FPRLocationQueryCandidate& Candidate : Candidates)
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
			for (const FPRLocationQueryCandidate& Candidate : Candidates)
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
}

UBTTask_PRPickReachableLocation::UBTTask_PRPickReachableLocation()
{
	NodeName = TEXT("Pick Reachable Location");
}

EBTNodeResult::Type UBTTask_PRPickReachableLocation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* ControlledPawn = IsValid(AIController) ? AIController->GetPawn() : nullptr;
	if (!IsValid(BlackboardComponent))
	{
		return EBTNodeResult::Failed;
	}

	FVector OriginLocation = FVector::ZeroVector;
	if (!ResolveOriginLocation(BlackboardComponent, OriginLocation))
	{
		return EBTNodeResult::Failed;
	}

	FVector PickedLocation = FVector::ZeroVector;
	if (PickEQSLocation(ControlledPawn, PickedLocation))
	{
		return WriteGoalLocation(BlackboardComponent, PickedLocation)
			? EBTNodeResult::Succeeded
			: EBTNodeResult::Failed;
	}

	if (bFallbackToNavmeshRandom
		&& PickReachableLocation(BlackboardComponent, OriginLocation, PickedLocation))
	{
		return WriteGoalLocation(BlackboardComponent, PickedLocation)
			? EBTNodeResult::Succeeded
			: EBTNodeResult::Failed;
	}

	if (bFallbackToOrigin)
	{
		return WriteGoalLocation(BlackboardComponent, OriginLocation)
			? EBTNodeResult::Succeeded
			: EBTNodeResult::Failed;
	}

	return EBTNodeResult::Failed;
}

FString UBTTask_PRPickReachableLocation::GetStaticDescription() const
{
	return FString::Printf(
		TEXT("%s\nOrigin Key: %s\nMove Goal Key: %s\nEQS: %s\nEQS Fallback Navmesh: %s\nRadius: %.1f\nMin Distance: %.1f\nAttempts: %d\nFallback To Origin: %s"),
		*Super::GetStaticDescription(),
		*OriginLocationKey.ToString(),
		*MoveGoalLocationKey.ToString(),
		*GetNameSafe(EQSQueryConfig.QueryTemplate),
		bFallbackToNavmeshRandom ? TEXT("true") : TEXT("false"),
		PickRadius,
		MinDistanceFromOrigin,
		MaxPickAttempts,
		bFallbackToOrigin ? TEXT("true") : TEXT("false"));
}

bool UBTTask_PRPickReachableLocation::ResolveOriginLocation(const UBlackboardComponent* BlackboardComponent, FVector& OutOriginLocation) const
{
	if (!IsValid(BlackboardComponent) || OriginLocationKey == NAME_None)
	{
		return false;
	}

	if (BlackboardComponent->GetKeyID(OriginLocationKey) == FBlackboard::InvalidKey)
	{
		return false;
	}

	if (!BlackboardComponent->IsVectorValueSet(OriginLocationKey))
	{
		return false;
	}

	OutOriginLocation = BlackboardComponent->GetValueAsVector(OriginLocationKey);
	return true;
}

bool UBTTask_PRPickReachableLocation::PickEQSLocation(APawn* ControlledPawn, FVector& OutPickedLocation) const
{
	if (!IsValid(ControlledPawn) || !IsValid(EQSQueryConfig.QueryTemplate))
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

	FEnvQueryRequest QueryRequest(EQSQueryConfig.QueryTemplate, ControlledPawn);
	for (const FPREnemyEQSFloatParam& FloatParam : EQSQueryConfig.FloatParams)
	{
		if (FloatParam.ParamName == NAME_None)
		{
			continue;
		}

		QueryRequest.SetFloatParam(FloatParam.ParamName, FloatParam.Value);
	}

	const TSharedPtr<FEnvQueryResult> QueryResult = QueryManager->RunInstantQuery(
		QueryRequest,
		ResolveQueryRunMode(EQSQueryConfig));
	if (!QueryResult.IsValid() || !QueryResult->IsSuccessful() || QueryResult->Items.Num() <= 0)
	{
		return false;
	}

	TArray<FPRLocationQueryCandidate> Candidates;
	BuildSortedCandidates(*QueryResult, Candidates);
	FilterTopCandidates(EQSQueryConfig, Candidates);

	int32 SelectedItemIndex = INDEX_NONE;
	if (!SelectCandidateIndex(EQSQueryConfig, Candidates, SelectedItemIndex) || SelectedItemIndex == INDEX_NONE)
	{
		return false;
	}

	OutPickedLocation = QueryResult->GetItemAsLocation(SelectedItemIndex);
	return true;
}

bool UBTTask_PRPickReachableLocation::PickReachableLocation(
	const UObject* WorldContextObject,
	const FVector& OriginLocation,
	FVector& OutPickedLocation) const
{
	if (!IsValid(WorldContextObject))
	{
		return false;
	}

	if (PickRadius <= KINDA_SMALL_NUMBER)
	{
		OutPickedLocation = OriginLocation;
		return true;
	}

	UWorld* World = WorldContextObject->GetWorld();
	if (!IsValid(World))
	{
		return false;
	}

	UNavigationSystemV1* NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	if (!IsValid(NavigationSystem))
	{
		return false;
	}

	ANavigationData* NavigationData = NavigationSystem->GetDefaultNavDataInstance(FNavigationSystem::DontCreate);
	FSharedConstNavQueryFilter QueryFilter = nullptr;
	if (IsValid(NavigationData) && FilterClass != nullptr)
	{
		QueryFilter = UNavigationQueryFilter::GetQueryFilter(*NavigationData, FilterClass);
	}

	const int32 AttemptCount = FMath::Max(MaxPickAttempts, 1);
	for (int32 AttemptIndex = 0; AttemptIndex < AttemptCount; ++AttemptIndex)
	{
		FNavLocation CandidateLocation;
		const bool bHasCandidate = NavigationSystem->GetRandomReachablePointInRadius(
			OriginLocation,
			PickRadius,
			CandidateLocation,
			NavigationData,
			QueryFilter);

		if (!bHasCandidate)
		{
			continue;
		}

		if (MinDistanceFromOrigin > 0.0f
			&& FVector::Dist2D(OriginLocation, CandidateLocation.Location) < MinDistanceFromOrigin)
		{
			continue;
		}

		OutPickedLocation = CandidateLocation.Location;
		return true;
	}

	return false;
}

bool UBTTask_PRPickReachableLocation::WriteGoalLocation(UBlackboardComponent* BlackboardComponent, const FVector& GoalLocation) const
{
	if (!IsValid(BlackboardComponent) || MoveGoalLocationKey == NAME_None)
	{
		return false;
	}

	if (BlackboardComponent->GetKeyID(MoveGoalLocationKey) == FBlackboard::InvalidKey)
	{
		return false;
	}

	BlackboardComponent->SetValueAsVector(MoveGoalLocationKey, GoalLocation);
	return true;
}

// Copyright ProjectR. All Rights Reserved.

#include "PRGameplayAbility_BossSpawnPatternActors.h"

#include "Engine/World.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "ProjectR/AI/Boss/PRBossPatternActor.h"
#include "ProjectR/Character/Enemy/PRBossBaseCharacter.h"

DEFINE_LOG_CATEGORY_STATIC(LogPRBossSpawnPatternActors, Log, All);

namespace
{
	struct FPRBossSpawnQueryCandidate
	{
		int32 ItemIndex = INDEX_NONE;
		float Score = 0.0f;
	};

	EEnvQueryRunMode::Type ResolveSpawnQueryRunMode(const FPRBossPatternActorSpawnConfig& SpawnConfig)
	{
		if (SpawnConfig.CandidateSelectionMode == EPREnemyQueryCandidateSelectionMode::BestScore)
		{
			return SpawnConfig.SpawnQueryRunMode;
		}

		return EEnvQueryRunMode::AllMatching;
	}

	void BuildSortedCandidates(const FEnvQueryResult& QueryResult, TArray<FPRBossSpawnQueryCandidate>& OutCandidates)
	{
		for (int32 ItemIndex = 0; ItemIndex < QueryResult.Items.Num(); ++ItemIndex)
		{
			if (!QueryResult.Items[ItemIndex].IsValid())
			{
				continue;
			}

			FPRBossSpawnQueryCandidate Candidate;
			Candidate.ItemIndex = ItemIndex;
			Candidate.Score = QueryResult.GetItemScore(ItemIndex);
			OutCandidates.Add(Candidate);
		}

		OutCandidates.Sort([](const FPRBossSpawnQueryCandidate& Left, const FPRBossSpawnQueryCandidate& Right)
		{
			return Left.Score > Right.Score;
		});
	}

	void FilterTopCandidates(const FPRBossPatternActorSpawnConfig& SpawnConfig, TArray<FPRBossSpawnQueryCandidate>& Candidates)
	{
		if (Candidates.IsEmpty())
		{
			return;
		}

		const float MaxScore = Candidates[0].Score;
		const float MinAllowedScore = MaxScore * FMath::Clamp(SpawnConfig.TopScoreCandidateRatio, 0.0f, 1.0f);
		Candidates.RemoveAll([MinAllowedScore](const FPRBossSpawnQueryCandidate& Candidate)
		{
			return Candidate.Score < MinAllowedScore;
		});

		if (SpawnConfig.TopCandidateCount > 0 && Candidates.Num() > SpawnConfig.TopCandidateCount)
		{
			Candidates.SetNum(SpawnConfig.TopCandidateCount);
		}
	}

	bool SelectCandidateIndex(
		const FPRBossPatternActorSpawnConfig& SpawnConfig,
		const TArray<FPRBossSpawnQueryCandidate>& Candidates,
		int32& OutItemIndex)
	{
		if (Candidates.IsEmpty())
		{
			return false;
		}

		switch (SpawnConfig.CandidateSelectionMode)
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
			for (const FPRBossSpawnQueryCandidate& Candidate : Candidates)
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
			for (const FPRBossSpawnQueryCandidate& Candidate : Candidates)
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

UPRGameplayAbility_BossSpawnPatternActors::UPRGameplayAbility_BossSpawnPatternActors()
{
}

void UPRGameplayAbility_BossSpawnPatternActors::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CanRunBossPattern() || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	SpawnedPatternActors.Reset();

	for (const FPRBossPatternActorSpawnConfig& SpawnConfig : PatternActorSpawnConfigs)
	{
		if (APRBossPatternActor* SpawnedActor = SpawnPatternActor(SpawnConfig))
		{
			SpawnedPatternActors.Add(SpawnedActor);
		}
	}

	TArray<APRBossPatternActor*> SpawnedActorsForEvent;
	SpawnedActorsForEvent.Reserve(SpawnedPatternActors.Num());
	for (APRBossPatternActor* SpawnedActor : SpawnedPatternActors)
	{
		if (IsValid(SpawnedActor))
		{
			SpawnedActorsForEvent.Add(SpawnedActor);
		}
	}

	BP_OnPatternActorsSpawned(SpawnedActorsForEvent);
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

bool UPRGameplayAbility_BossSpawnPatternActors::BuildPatternActorSpawnTransform(
	const FPRBossPatternActorSpawnConfig& SpawnConfig,
	FTransform& OutSpawnTransform) const
{
	if (SpawnConfig.SpawnLocationMode != EPRBossPatternSpawnLocationMode::EnvQuery)
	{
		return BuildOriginOffsetSpawnTransform(SpawnConfig, OutSpawnTransform);
	}

	FVector SpawnLocation = FVector::ZeroVector;
	if (!RunSpawnLocationQuery(SpawnConfig, SpawnLocation))
	{
		if (SpawnConfig.bFallbackToOriginOffsetWhenQueryFails)
		{
			return BuildOriginOffsetSpawnTransform(SpawnConfig, OutSpawnTransform);
		}

		return false;
	}

	const APRBossBaseCharacter* BossCharacter = GetBossAvatarCharacter();
	AActor* PatternTarget = GetBossPatternTarget();
	if (!IsValid(BossCharacter))
	{
		return false;
	}

	FRotator SpawnRotation = BossCharacter->GetActorRotation();
	if (SpawnConfig.bFaceTarget && IsValid(PatternTarget))
	{
		const FVector DirectionToTarget = PatternTarget->GetActorLocation() - SpawnLocation;
		if (!DirectionToTarget.IsNearlyZero())
		{
			SpawnRotation = FRotationMatrix::MakeFromX(DirectionToTarget).Rotator();
			if (SpawnConfig.bUseYawOnlyFacing)
			{
				SpawnRotation.Pitch = 0.0f;
				SpawnRotation.Roll = 0.0f;
			}
		}
	}

	OutSpawnTransform = FTransform(SpawnRotation + SpawnConfig.RotationOffset, SpawnLocation);
	return true;
}

bool UPRGameplayAbility_BossSpawnPatternActors::BuildOriginOffsetSpawnTransform(
	const FPRBossPatternActorSpawnConfig& SpawnConfig,
	FTransform& OutSpawnTransform) const
{
	const APRBossBaseCharacter* BossCharacter = GetBossAvatarCharacter();
	AActor* PatternTarget = GetBossPatternTarget();

	if (!IsValid(BossCharacter))
	{
		return false;
	}

	const AActor* OriginActor = BossCharacter;
	if (SpawnConfig.SpawnOrigin == EPRBossPatternSpawnOrigin::Target && IsValid(PatternTarget))
	{
		OriginActor = PatternTarget;
	}

	const FTransform OriginTransform = OriginActor->GetActorTransform();
	const FVector SpawnLocation = OriginTransform.TransformPositionNoScale(SpawnConfig.LocalOffset);

	FRotator SpawnRotation = OriginTransform.GetRotation().Rotator();
	if (SpawnConfig.bFaceTarget && IsValid(PatternTarget))
	{
		const FVector DirectionToTarget = PatternTarget->GetActorLocation() - SpawnLocation;
		if (!DirectionToTarget.IsNearlyZero())
		{
			SpawnRotation = FRotationMatrix::MakeFromX(DirectionToTarget).Rotator();
			if (SpawnConfig.bUseYawOnlyFacing)
			{
				SpawnRotation.Pitch = 0.0f;
				SpawnRotation.Roll = 0.0f;
			}
		}
	}

	OutSpawnTransform = FTransform(SpawnRotation + SpawnConfig.RotationOffset, SpawnLocation);
	return true;
}

bool UPRGameplayAbility_BossSpawnPatternActors::RunSpawnLocationQuery(
	const FPRBossPatternActorSpawnConfig& SpawnConfig,
	FVector& OutSpawnLocation) const
{
	APRBossBaseCharacter* BossCharacter = GetBossAvatarCharacter();
	if (!IsValid(BossCharacter) || !IsValid(SpawnConfig.SpawnQueryTemplate))
	{
		return false;
	}

	UWorld* World = BossCharacter->GetWorld();
	if (!IsValid(World))
	{
		return false;
	}

	UEnvQueryManager* QueryManager = UEnvQueryManager::GetCurrent(World);
	if (!IsValid(QueryManager))
	{
		return false;
	}

	FEnvQueryRequest QueryRequest(SpawnConfig.SpawnQueryTemplate, BossCharacter);
	for (const FPREnemyEQSFloatParam& FloatParam : SpawnConfig.FloatParams)
	{
		if (FloatParam.ParamName == NAME_None)
		{
			continue;
		}

		QueryRequest.SetFloatParam(FloatParam.ParamName, FloatParam.Value);
	}

	const TSharedPtr<FEnvQueryResult> QueryResult = QueryManager->RunInstantQuery(
		QueryRequest,
		ResolveSpawnQueryRunMode(SpawnConfig));
	if (!QueryResult.IsValid() || !QueryResult->IsSuccessful() || QueryResult->Items.Num() <= 0)
	{
		UE_LOG(LogPRBossSpawnPatternActors, Verbose,
			TEXT("Boss pattern spawn EQS failed. Ability=%s, Query=%s"),
			*GetNameSafe(this),
			*GetNameSafe(SpawnConfig.SpawnQueryTemplate.Get()));
		return false;
	}

	TArray<FPRBossSpawnQueryCandidate> Candidates;
	BuildSortedCandidates(*QueryResult, Candidates);
	FilterTopCandidates(SpawnConfig, Candidates);

	int32 SelectedItemIndex = INDEX_NONE;
	if (!SelectCandidateIndex(SpawnConfig, Candidates, SelectedItemIndex) || SelectedItemIndex == INDEX_NONE)
	{
		return false;
	}

	OutSpawnLocation = QueryResult->GetItemAsLocation(SelectedItemIndex);
	return true;
}

APRBossPatternActor* UPRGameplayAbility_BossSpawnPatternActors::SpawnPatternActor(
	const FPRBossPatternActorSpawnConfig& SpawnConfig)
{
	APRBossBaseCharacter* BossCharacter = GetBossAvatarCharacter();
	if (!IsValid(BossCharacter) || !BossCharacter->HasAuthority() || !SpawnConfig.PatternActorClass)
	{
		return nullptr;
	}

	UWorld* World = BossCharacter->GetWorld();
	if (!IsValid(World))
	{
		return nullptr;
	}

	FTransform SpawnTransform;
	if (!BuildPatternActorSpawnTransform(SpawnConfig, SpawnTransform))
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = BossCharacter;
	SpawnParameters.Instigator = BossCharacter;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	APRBossPatternActor* SpawnedActor = World->SpawnActor<APRBossPatternActor>(
		SpawnConfig.PatternActorClass,
		SpawnTransform,
		SpawnParameters);

	if (IsValid(SpawnedActor))
	{
		SpawnedActor->InitializeBossPatternActor(BossCharacter, GetBossPatternTarget());
	}

	return SpawnedActor;
}

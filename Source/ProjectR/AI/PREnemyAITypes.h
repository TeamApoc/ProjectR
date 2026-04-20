// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "PREnemyAITypes.generated.h"

class AActor;

UENUM(BlueprintType)
enum class EPRTacticalMode : uint8
{
	Idle		UMETA(DisplayName = "Idle"),
	Alert		UMETA(DisplayName = "Alert"),
	Chase		UMETA(DisplayName = "Chase"),
	Attack		UMETA(DisplayName = "Attack"),
	Reposition	UMETA(DisplayName = "Reposition"),
	Return		UMETA(DisplayName = "Return")
};

UENUM(BlueprintType)
enum class EPRFaerinPhase : uint8
{
	Opening				UMETA(DisplayName = "Opening"),
	AdvancedRanged		UMETA(DisplayName = "AdvancedRanged"),
	SwordJudgment		UMETA(DisplayName = "SwordJudgment"),
	FinalTeleportLoop	UMETA(DisplayName = "FinalTeleportLoop")
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRThreatEntry
{
	GENERATED_BODY()

	// Threat 대상 액터다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI")
	TObjectPtr<AActor> Target = nullptr;

	// 누적 Threat 값이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI")
	float ThreatValue = 0.0f;

	// 마지막 갱신 시각이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI")
	float LastUpdatedTime = 0.0f;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRPatternContext
{
	GENERATED_BODY()

	// 현재 타겟까지의 거리다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI")
	float DistanceToTarget = 0.0f;

	// 현재 LOS 확보 여부다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI")
	bool bHasLOS = false;

	// 현재 전술 모드다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI")
	EPRTacticalMode TacticalMode = EPRTacticalMode::Idle;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRPatternRule
{
	GENERATED_BODY()

	// 실행할 어빌리티 식별 태그다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI")
	FGameplayTag AbilityTag;

	// 패턴 최소 유효 거리다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI", meta = (ClampMin = "0.0"))
	float MinRange = 0.0f;

	// 패턴 최대 유효 거리다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI", meta = (ClampMin = "0.0"))
	float MaxRange = 3000.0f;

	// LOS 필요 여부다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI")
	bool bRequiresLOS = true;

	// 조건을 통과한 후보 간 선택 가중치다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI", meta = (ClampMin = "0.0"))
	float SelectionWeight = 1.0f;

	bool IsValid() const
	{
		return AbilityTag.IsValid()
			&& MinRange <= MaxRange
			&& SelectionWeight > 0.0f;
	}

	bool MatchesContext(const FPRPatternContext& Context) const
	{
		if (!IsValid())
		{
			return false;
		}

		const bool bInRange = Context.DistanceToTarget >= MinRange
			&& Context.DistanceToTarget <= MaxRange;
		if (!bInRange)
		{
			return false;
		}

		if (bRequiresLOS && !Context.bHasLOS)
		{
			return false;
		}

		return true;
	}
};

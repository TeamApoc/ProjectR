// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "PREnemyAITypes.generated.h"

class AActor;

// Blackboard에 저장되는 몬스터의 큰 전술 상태다.
// 세부 패턴 선택은 BT/Ability가 담당하고, 이 값은 AI가 현재 무엇을 하려는지 공유하는 기준으로 쓴다.
UENUM(BlueprintType)
enum class EPRTacticalMode : uint8
{
	Idle		UMETA(DisplayName = "Idle"),
	Alert		UMETA(DisplayName = "Alert"),
	FastApproach	UMETA(DisplayName = "FastApproach"),
	Attack		UMETA(DisplayName = "Attack"),
	Strafe		UMETA(DisplayName = "Strafe"),
	Return		UMETA(DisplayName = "Return")
};

// 패턴의 전투 계열이다.
// BT 브랜치가 "근접", "돌진"처럼 큰 흐름을 먼저 나누고 DataAsset 규칙으로 세부 패턴을 고르게 한다.
UENUM(BlueprintType)
enum class EPRPatternCategory : uint8
{
	Any			UMETA(DisplayName = "Any"),
	Melee		UMETA(DisplayName = "Melee"),
	Sprint		UMETA(DisplayName = "Sprint"),
	Ranged		UMETA(DisplayName = "Ranged"),
	Movement	UMETA(DisplayName = "Movement"),
	Special		UMETA(DisplayName = "Special")
};

// 패턴 컨텍스트 비교 시 어떤 조건을 무시할지 정의한다.
UENUM(BlueprintType)
enum class EPRPatternContextMatchMode : uint8
{
	FullMatch	UMETA(DisplayName = "FullMatch"),
	IgnoreRange	UMETA(DisplayName = "IgnoreRange")
};

// Perception이 타겟을 잃었을 때 Threat/Blackboard를 어디까지 정리할지 정한다.
// 적 종류마다 수색 집착도가 다를 수 있으므로 AIController 설정값으로 사용한다.
UENUM(BlueprintType)
enum class EPRTargetLostPolicy : uint8
{
	KeepCurrentTarget	UMETA(DisplayName = "Keep Current Target"),
	ClearCurrentTarget	UMETA(DisplayName = "Clear Current Target"),
	RemoveThreatEntry	UMETA(DisplayName = "Remove Threat Entry")
};

// Faerin 보스 전용 페이즈 값이다.
// 페이즈 전환 Ability와 이벤트 릴레이가 같은 기준을 보도록 공용 타입으로 둔다.
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

	// 위협을 누적할 대상 액터다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI")
	TObjectPtr<AActor> Target = nullptr;

	// 대상에게 누적된 위협 수치다. 가장 높은 대상이 현재 타겟 후보가 된다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI")
	float ThreatValue = 0.0f;

	// 마지막으로 위협이 갱신된 시간이다. 오래된 항목을 잊어버릴 때 사용한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI")
	float LastUpdatedTime = 0.0f;
};

// 패턴 선택 시점에 필요한 상황값만 모아둔 구조체다.
// BTTask가 Blackboard와 Pawn 상태를 읽어 채운 뒤 FPRPatternRule과 비교한다.
USTRUCT(BlueprintType)
struct PROJECTR_API FPRPatternContext
{
	GENERATED_BODY()

	// 현재 타겟까지의 거리다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI")
	float DistanceToTarget = 0.0f;

	// 타겟을 시야선으로 확인할 수 있는지 여부다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI")
	bool bHasLOS = false;

	// 현재 전술 상태다. 지금은 거리/LOS 중심이지만 이후 패턴 조건 확장 지점이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI")
	EPRTacticalMode TacticalMode = EPRTacticalMode::Idle;

	// 돌진 계열 패턴이 실제로 지나갈 수 있는 직선 경로를 확보했는지 여부다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI")
	bool bChargePathClear = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI")
	float CurrentAttackPressure = 0.0f;

};

// 하나의 몬스터 패턴 후보를 정의한다.
// 조건에 맞는 후보들만 모은 뒤 SelectionWeight 비율로 최종 Ability를 고른다.
USTRUCT(BlueprintType)
struct PROJECTR_API FPRPatternRule
{
	GENERATED_BODY()

	// 실행할 Ability를 찾는 GameplayTag다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI")
	FGameplayTag AbilityTag;

	// BT에서 큰 공격 흐름을 나눌 때 사용하는 패턴 계열이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI")
	EPRPatternCategory PatternCategory = EPRPatternCategory::Any;

	// 이 패턴이 유효한 최소 거리다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI", meta = (ClampMin = "0.0"))
	float MinRange = 0.0f;

	// 이 패턴이 유효한 최대 거리다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI", meta = (ClampMin = "0.0"))
	float MaxRange = 3000.0f;

	// true면 타겟이 시야선 안에 있을 때만 선택된다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI")
	bool bRequiresLOS = true;

	// true면 Blackboard의 charge_path_clear가 켜져 있을 때만 선택된다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI")
	bool bRequiresChargePathClear = false;

	// true면 AllowedTacticalModes에 들어 있는 전술 상태에서만 선택된다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI")
	bool bRestrictTacticalModes = false;

	// 이 패턴을 허용할 전술 상태 목록이다. 비워두면 bRestrictTacticalModes가 false일 때만 의미가 없다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI", meta = (EditCondition = "bRestrictTacticalModes"))
	TArray<EPRTacticalMode> AllowedTacticalModes;

	// 조건을 통과한 후보끼리의 선택 가중치다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI", meta = (ClampMin = "0.0"))
	float SelectionWeight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI", meta = (ClampMin = "0.0"))
	float RequiredAttackPressure = 0.0f;

	// 데이터 에셋에서 잘못된 패턴이 들어왔는지 빠르게 걸러낸다.
	bool IsValid() const
	{
		return AbilityTag.IsValid()
			&& MinRange <= MaxRange
			&& SelectionWeight > 0.0f;
	}

	// 현재 상황값이 이 패턴의 거리/시야 조건을 만족하는지 확인한다.
	bool MatchesContext(
		const FPRPatternContext& Context,
		const EPRPatternContextMatchMode MatchMode = EPRPatternContextMatchMode::FullMatch) const
	{
		if (!IsValid())
		{
			return false;
		}

		if (MatchMode != EPRPatternContextMatchMode::IgnoreRange)
		{
			const bool bInRange = Context.DistanceToTarget >= MinRange
				&& Context.DistanceToTarget <= MaxRange;
			if (!bInRange)
			{
				return false;
			}
		}

		if (bRequiresLOS && !Context.bHasLOS)
		{
			return false;
		}

		if (bRequiresChargePathClear && !Context.bChargePathClear)
		{
			return false;
		}

		if (bRestrictTacticalModes && !AllowedTacticalModes.Contains(Context.TacticalMode))
		{
			return false;
		}

		if (Context.CurrentAttackPressure < RequiredAttackPressure)
		{
			return false;
		}

		return true;
	}
};

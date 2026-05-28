// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "GameplayTagContainer.h"
#include "ProjectR/AI/Data/PREnemyCombatDataAsset.h"
#include "ProjectR/AI/PREnemyAITypes.h"
#include "PRFaerinCombatLoopDataAsset.generated.h"

class UEnvQuery;
class UPRAbilitySystemComponent;
class UPRPatternDataAsset;

UENUM(BlueprintType)
enum class EPRFaerinTeleportWrapperPolicy : uint8
{
	None				UMETA(DisplayName = "None"),
	TeleportOutOnly		UMETA(DisplayName = "Teleport Out Only"),
	TeleportOutAndIn	UMETA(DisplayName = "Teleport Out And In"),
	TeleportOutVFXOnly	UMETA(DisplayName = "Teleport Out VFX Only")
};

UENUM(BlueprintType)
enum class EPRFaerinApproachPolicy : uint8
{
	PhaseDefault		UMETA(DisplayName = "Phase Default"),
	None				UMETA(DisplayName = "None"),
	KeepCurrentRange	UMETA(DisplayName = "Keep Current Range"),
	SprintToMeleeRange	UMETA(DisplayName = "Sprint To Melee Range"),
	ShiftClose			UMETA(DisplayName = "Shift Close"),
	NearTeleportToMeleeRange	UMETA(DisplayName = "Near Teleport To Melee Range"),
	SprintOrNearTeleport	UMETA(DisplayName = "Sprint Or Near Teleport")
};

UENUM(BlueprintType)
enum class EPRFaerinPostPatternPolicy : uint8
{
	PhaseDefault		UMETA(DisplayName = "Phase Default"),
	ForceStrafe			UMETA(DisplayName = "Force Strafe"),
	ForceImmediateNext	UMETA(DisplayName = "Force Immediate Next")
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRFaerinPhaseLoopConfig
{
	GENERATED_BODY()

	// 이 설정이 적용되는 보스 페이즈다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin")
	EPRBossPhase Phase = EPRBossPhase::Phase1;

	// 패턴 종료 후 원작형 횡이동 구간을 사용할지 결정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin")
	bool bUsePostPatternStrafe = true;

	// 체력이 높은 기준점에서 사용할 횡이동 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin", meta = (ClampMin = "0.0"))
	float StrafeDurationAtHighHealth = 1.35f;

	// 체력이 낮은 기준점에서 사용할 횡이동 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin", meta = (ClampMin = "0.0"))
	float StrafeDurationAtLowHealth = 0.55f;

	// 높은 체력 시간 보간에 사용할 체력 비율 기준점이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float HighHealthRatioReference = 1.0f;

	// 낮은 체력 시간 보간에 사용할 체력 비율 기준점이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LowHealthRatioReference = 0.25f;

	// 현재 거리 대신 강제로 유지할 횡이동 반경이다. 0이면 현재 거리 기반으로 계산한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin", meta = (ClampMin = "0.0"))
	float StrafeRadiusOverride = 0.0f;

	// 한 번의 횡이동에서 타깃을 기준으로 회전할 각도다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin", meta = (ClampMin = "1.0", ClampMax = "120.0"))
	float StrafeArcAngleDegrees = 34.0f;

	// AI MoveTo 도착 판정 반경이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin", meta = (ClampMin = "0.0"))
	float StrafeAcceptanceRadius = 80.0f;

	// 횡이동 목적지를 NavMesh 위로 보정할 때 사용하는 검색 범위다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin", meta = (ClampMin = "0.0"))
	FVector StrafeNavProjectExtent = FVector(220.0f, 220.0f, 360.0f);

	// 연속 횡이동 시 좌우 방향을 번갈아 사용할지 결정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin")
	bool bAlternateStrafeDirection = true;

	// 횡이동 중 애니메이션/이동 표현에 적용할 설정이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin")
	FPREnemyMovePresentationConfig StrafePresentationConfig;

	// 횡이동 이후 다음 공격을 준비하는 접근 단계를 사용할지 결정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin")
	bool bUsePostStrafeApproach = true;

	// 스프린트 접근을 실제로 실행할 Gameplay Ability 태그다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin", meta = (Categories = "Ability.Boss.Faerin"))
	FGameplayTag ApproachAbilityTag;

	// 근거리 텔레포트 접근을 실제로 실행할 Gameplay Ability 태그다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin", meta = (Categories = "Ability.Boss.Faerin"))
	FGameplayTag NearTeleportAbilityTag;

	// 패턴 메타데이터가 PhaseDefault를 선택했을 때 사용할 기본 접근 정책이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin")
	EPRFaerinApproachPolicy DefaultApproachPolicy = EPRFaerinApproachPolicy::SprintToMeleeRange;

	// 이 거리보다 멀 때 sprint 접근을 시작한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin", meta = (ClampMin = "0.0"))
	float ApproachTriggerDistance = 1400.0f;

	// sprint 접근으로 유지하려는 타깃과의 거리다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin", meta = (ClampMin = "0.0"))
	float ApproachStopDistance = 500.0f;

	// 접근 이동을 유지할 최대 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin", meta = (ClampMin = "0.0"))
	float ApproachTimeoutSeconds = 0.75f;

	// AI MoveTo 접근 도착 판정 반경이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin", meta = (ClampMin = "0.0"))
	float ApproachAcceptanceRadius = 120.0f;

	// SprintOrNearTeleport 정책에서 근거리 텔레포트를 선택할 확률이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float NearTeleportChance = 0.35f;

	// 근거리 텔레포트가 사라진 뒤 자기 주변 위치에 재등장하기까지 기다리는 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin", meta = (ClampMin = "0.0"))
	float NearTeleportReappearDelaySeconds = 0.5f;

	// 자기 위치 기준 근거리 텔레포트가 이동할 수 있는 최대 거리다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin", meta = (ClampMin = "0.0", ClampMax = "500.0"))
	float NearTeleportMaxDistanceFromSelf = 500.0f;

	// 자기 주변 근거리 텔레포트 목적지를 고를 EQS다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|NearTeleport|EQS")
	TObjectPtr<UEnvQuery> NearTeleportQueryTemplate;

	// 근거리 텔레포트 EQS 실행 방식이다. 후보 랜덤 선택을 쓰면 내부 유틸에서 AllMatching으로 보정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|NearTeleport|EQS")
	TEnumAsByte<EEnvQueryRunMode::Type> NearTeleportQueryRunMode = EEnvQueryRunMode::SingleResult;

	// 근거리 텔레포트 EQS 후보 선택 방식이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|NearTeleport|EQS")
	EPREnemyQueryCandidateSelectionMode NearTeleportCandidateSelectionMode = EPREnemyQueryCandidateSelectionMode::RandomTopCandidates;

	// 근거리 텔레포트 EQS Named Float Param 목록이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|NearTeleport|EQS")
	TArray<FPREnemyEQSFloatParam> NearTeleportFloatParams;

	// 근거리 텔레포트 상위 후보 최대 선택 개수다. 0 이하면 개수 제한이 없다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|NearTeleport|EQS", meta = (ClampMin = "0"))
	int32 NearTeleportTopCandidateCount = 5;

	// 근거리 텔레포트 최고 점수 대비 유지할 최소 점수 비율이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|NearTeleport|EQS", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float NearTeleportTopScoreCandidateRatio = 0.85f;

	// 접근 목적지를 NavMesh 위로 보정할 때 사용하는 검색 범위다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin", meta = (ClampMin = "0.0"))
	FVector ApproachNavProjectExtent = FVector(240.0f, 240.0f, 360.0f);

	// 접근 중 애니메이션/이동 표현에 적용할 설정이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin")
	FPREnemyMovePresentationConfig ApproachPresentationConfig;

	// 본 공격 전에 보조 포털 패턴을 확률적으로 끼워 넣을지 결정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|PortalAssist")
	bool bUsePrePatternPortalAssist = false;

	// 보조 포털 패턴을 본 공격 앞에 붙일 확률이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|PortalAssist", meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bUsePrePatternPortalAssist"))
	float PrePatternPortalAssistChance = 0.5f;

	// 본 공격이 포털 계열일 때도 선행 포털 보조를 허용할지 결정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|PortalAssist", meta = (EditCondition = "bUsePrePatternPortalAssist"))
	bool bAllowPrePatternPortalAssistBeforePortalPatterns = true;

	// 보조 포털 후보로 사용할 PatternGroup이다. 비워 두면 Faerin Portal 그룹을 사용한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|PortalAssist", meta = (Categories = "Pattern.Boss.Faerin", EditCondition = "bUsePrePatternPortalAssist"))
	FGameplayTag PrePatternPortalGroupTag;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRFaerinPatternLoopMetadata
{
	GENERATED_BODY()

	// PatternData의 AbilityTag와 1:1로 대응되는 패턴 식별자다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin", meta = (Categories = "Ability.Boss.Faerin"))
	FGameplayTag AbilityTag;

	// 현재 리팩터링 루프에서 선택 가능한 패턴인지 결정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin")
	bool bEnabled = true;

	// 선택 가중치에 추가로 곱할 보정값이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin", meta = (ClampMin = "0.0"))
	float SelectionWeightScale = 1.0f;

	// 원작 재현용 텔레포트 전/후처리 정책이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin")
	EPRFaerinTeleportWrapperPolicy TeleportWrapperPolicy = EPRFaerinTeleportWrapperPolicy::None;

	// 공격 종료 후 다음 루프를 준비하는 접근 방식이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin")
	EPRFaerinApproachPolicy ApproachPolicy = EPRFaerinApproachPolicy::PhaseDefault;

	// 공격 종료 후 루프 진행 방식이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin")
	EPRFaerinPostPatternPolicy PostPatternPolicy = EPRFaerinPostPatternPolicy::PhaseDefault;

	// 설계 검토용 메모다. 런타임 판정에는 사용하지 않는다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin")
	FString DesignNote;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRFaerinPatternPlan
{
	GENERATED_BODY()

	// 이번 루프에서 실행할 AbilityTag다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin")
	FGameplayTag AbilityTag;

	// PatternData에서 복사한 선택 규칙이다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin")
	FPRPatternRule PatternRule;

	// LoopData에서 복사한 실행 메타데이터다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin")
	FPRFaerinPatternLoopMetadata LoopMetadata;

	// 최종 선택에 사용한 가중치다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin")
	float FinalSelectionWeight = 0.0f;
};

// Faerin 전용 전투 루프가 PatternData/AbilitySet을 원작형 순서로 엮는 데 사용하는 데이터다.
UCLASS(BlueprintType)
class PROJECTR_API UPRFaerinCombatLoopDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// 페이즈별 루프 설정을 찾는다.
	const FPRFaerinPhaseLoopConfig* FindPhaseConfig(EPRBossPhase Phase) const;

	// AbilityTag에 대응되는 패턴 메타데이터를 찾는다.
	const FPRFaerinPatternLoopMetadata* FindPatternMetadata(const FGameplayTag& AbilityTag) const;

	// PatternData/AbilitySet 정합성을 점검하고 발견된 문제를 반환한다.
	bool ValidateLoopData(
		const UPRPatternDataAsset* PatternDataAsset,
		UPRAbilitySystemComponent* AbilitySystemComponent,
		TArray<FString>& OutErrors) const;

public:
	// 페이즈별 공격 후 루프 설정이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin")
	TArray<FPRFaerinPhaseLoopConfig> PhaseConfigs;

	// PatternData의 Rule과 1:1 대응되는 패턴 실행 메타데이터다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin")
	TArray<FPRFaerinPatternLoopMetadata> PatternMetadata;
};

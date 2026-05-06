// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "ProjectR/AbilitySystem/Abilities/Boss/PRGameplayAbility_BossPatternBase.h"
#include "ProjectR/AI/Data/PREnemyCombatDataAsset.h"
#include "PRGameplayAbility_BossSpawnPatternActors.generated.h"

class APRBossPatternActor;
class UEnvQuery;

// 보스 패턴 Helper Actor를 어느 기준점에 생성할지 정의한다.
UENUM(BlueprintType)
enum class EPRBossPatternSpawnOrigin : uint8
{
	Boss	UMETA(DisplayName = "Boss"),
	Target	UMETA(DisplayName = "Target")
};

// 보스 패턴 Helper Actor의 위치 선정 방식이다.
UENUM(BlueprintType)
enum class EPRBossPatternSpawnLocationMode : uint8
{
	OriginOffset	UMETA(DisplayName = "OriginOffset"),
	EnvQuery		UMETA(DisplayName = "EnvQuery")
};

// 보스 패턴 Helper Actor 하나의 생성 설정이다.
USTRUCT(BlueprintType)
struct PROJECTR_API FPRBossPatternActorSpawnConfig
{
	GENERATED_BODY()

	// 생성할 공용 Helper Actor 또는 BP 파생 클래스다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss")
	TSubclassOf<APRBossPatternActor> PatternActorClass;

	// Helper Actor 위치 선정 방식이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss")
	EPRBossPatternSpawnLocationMode SpawnLocationMode = EPRBossPatternSpawnLocationMode::OriginOffset;

	// 생성 기준점이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss")
	EPRBossPatternSpawnOrigin SpawnOrigin = EPRBossPatternSpawnOrigin::Target;

	// 기준 Actor의 로컬 좌표계에서 적용할 위치 오프셋이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss")
	FVector LocalOffset = FVector::ZeroVector;

	// 계산된 회전에 추가로 적용할 회전 오프셋이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss")
	FRotator RotationOffset = FRotator::ZeroRotator;

	// 생성 위치에서 타겟을 바라보도록 회전을 보정할지 여부다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss")
	bool bFaceTarget = true;

	// 타겟 바라보기 회전에서 Pitch/Roll을 제거해 수평 회전만 사용할지 여부다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss", meta = (EditCondition = "bFaceTarget"))
	bool bUseYawOnlyFacing = true;

	// EnvQuery 위치 선정에 사용할 EQS 템플릿이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|EQS", meta = (EditCondition = "SpawnLocationMode == EPRBossPatternSpawnLocationMode::EnvQuery"))
	TObjectPtr<UEnvQuery> SpawnQueryTemplate = nullptr;

	// EQS 실행 방식이다. 후보 랜덤 선택을 쓰면 내부적으로 AllMatching으로 실행한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|EQS", meta = (EditCondition = "SpawnLocationMode == EPRBossPatternSpawnLocationMode::EnvQuery"))
	TEnumAsByte<EEnvQueryRunMode::Type> SpawnQueryRunMode = EEnvQueryRunMode::SingleResult;

	// EQS 후보 선택 방식이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|EQS", meta = (EditCondition = "SpawnLocationMode == EPRBossPatternSpawnLocationMode::EnvQuery"))
	EPREnemyQueryCandidateSelectionMode CandidateSelectionMode = EPREnemyQueryCandidateSelectionMode::BestScore;

	// EQS Named Float Param 목록이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|EQS", meta = (EditCondition = "SpawnLocationMode == EPRBossPatternSpawnLocationMode::EnvQuery"))
	TArray<FPREnemyEQSFloatParam> FloatParams;

	// 상위 후보 최대 선택 개수다. 0 이하면 개수 제한이 없다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|EQS", meta = (EditCondition = "SpawnLocationMode == EPRBossPatternSpawnLocationMode::EnvQuery", ClampMin = "0"))
	int32 TopCandidateCount = 0;

	// 최고 점수 대비 유지할 최소 점수 비율이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|EQS", meta = (EditCondition = "SpawnLocationMode == EPRBossPatternSpawnLocationMode::EnvQuery", ClampMin = "0.0", ClampMax = "1.0"))
	float TopScoreCandidateRatio = 0.9f;

	// EQS 실패 시 OriginOffset 방식으로 대체할지 여부다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|EQS", meta = (EditCondition = "SpawnLocationMode == EPRBossPatternSpawnLocationMode::EnvQuery"))
	bool bFallbackToOriginOffsetWhenQueryFails = true;
};

// 보스 패턴 Helper Actor들을 설정 배열 기준으로 생성하는 공용 Ability다.
// PortalPairSequence, SwordHazard 사전 배치처럼 지속형 Actor가 필요한 패턴의 첫 공용 진입점으로 사용한다.
UCLASS()
class PROJECTR_API UPRGameplayAbility_BossSpawnPatternActors : public UPRGameplayAbility_BossPatternBase
{
	GENERATED_BODY()

public:
	UPRGameplayAbility_BossSpawnPatternActors();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	// 지정한 설정으로 Helper Actor 생성 Transform을 계산한다.
	bool BuildPatternActorSpawnTransform(const FPRBossPatternActorSpawnConfig& SpawnConfig, FTransform& OutSpawnTransform) const;

	// OriginOffset 방식으로 Helper Actor 생성 Transform을 계산한다.
	bool BuildOriginOffsetSpawnTransform(const FPRBossPatternActorSpawnConfig& SpawnConfig, FTransform& OutSpawnTransform) const;

	// EQS 방식으로 Helper Actor 생성 위치를 계산한다.
	bool RunSpawnLocationQuery(const FPRBossPatternActorSpawnConfig& SpawnConfig, FVector& OutSpawnLocation) const;

	// Helper Actor를 하나 생성하고 초기화한다.
	APRBossPatternActor* SpawnPatternActor(const FPRBossPatternActorSpawnConfig& SpawnConfig);

	// Helper Actor 생성이 끝난 뒤 BP 후처리를 연결하는 이벤트다.
	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|AI|Boss")
	void BP_OnPatternActorsSpawned(const TArray<APRBossPatternActor*>& SpawnedActors);

protected:
	// 이 Ability가 생성할 Helper Actor 목록이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss")
	TArray<FPRBossPatternActorSpawnConfig> PatternActorSpawnConfigs;

	// 이번 활성화에서 생성된 Helper Actor 목록이다.
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|AI|Boss")
	TArray<TObjectPtr<APRBossPatternActor>> SpawnedPatternActors;
};

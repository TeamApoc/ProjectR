// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ProjectR/System/PRTickOptimizable.h"
#include "PRWorldTickOptimizerSubsystem.generated.h"

class AActor;

// 월드 Tick 최적화 대상의 평가 상태 보관 항목
USTRUCT()
struct FPRTickOptimizationEntry
{
	GENERATED_BODY()

	// 활성 목록의 실제 대상 액터
	TWeakObjectPtr<AActor> TargetActor;

	// 대상 액터의 인터페이스 캐스팅 비용을 줄이기 위한 캐시
	TScriptInterface<IPRTickOptimizable> TargetInterface;

	// Subsystem이 마지막으로 적용한 Tick 활성 상태
	bool bIsTickActive = true;

	// Subsystem이 마지막으로 적용한 Visibility 활성 상태
	bool bIsVisibilityActive = true;

	// 등록 시점에 읽은 거리 평가 설정값
	FPRTickOptimizationConfig Config;
};

// 한 평가 주기에서 재사용하는 플레이어 기준점
USTRUCT()
struct FPRTickOptimizationSource
{
	GENERATED_BODY()

	// 거리 평가 기준으로 사용하는 월드 위치
	FVector Location = FVector::ZeroVector;

	// 기준점이 된 플레이어 Pawn 또는 Actor
	TWeakObjectPtr<AActor> SourceActor;
};

// 월드 단위 거리 평가로 액터의 Tick 비용을 제어하는 서버 권위 Subsystem
UCLASS()
class PROJECTR_API UPRWorldTickOptimizerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/*~ UWorldSubsystem Interface ~*/
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

public:
	// Tick 최적화 대상 등록을 다음 평가 주기까지 예약
	void RegisterTarget(AActor* TargetActor);

	// Tick 최적화 대상 해제를 다음 평가 주기까지 예약
	void UnregisterTarget(AActor* TargetActor);

	// 현재 등록된 평가 대상 수 반환
	int32 GetRegisteredTargetCount() const { return ActiveEntries.Num(); }

	// 디버그와 테스트를 위한 예약 반영과 거리 평가 즉시 실행
	void ForceEvaluate();

protected:
	// 서버 월드 전용 평가 타이머 시작
	void StartEvaluationTimer();

	// 평가 타이머 정리
	void StopEvaluationTimer();

	// 예약된 등록과 해제의 활성 목록 일괄 반영
	void FlushPendingChanges();

	// 현재 월드의 유효한 플레이어 기준점 수집
	void BuildSources();

	// 등록된 대상의 활성 상태 평가
	void EvaluateTargets();

	// 단일 대상의 거리 조건 평가와 필요 전이 적용
	void EvaluateTarget(FPRTickOptimizationEntry& Entry);

	// 대상 액터와 인터페이스와 설정값 검증 후 평가 항목 생성
	bool BuildEntry(AActor* TargetActor, FPRTickOptimizationEntry& OutEntry) const;

	// 무효 대상과 해제 예약 대상의 활성 목록 제거
	void CompactEntries();

	// 플레이어 기준점 중 하나가 지정 반경 안에 있는지 여부 반환
	bool HasSourceInsideRadius(const FVector& TargetLocation, float Radius) const;

protected:
	// 실제 거리 평가에 사용하는 안정적인 대상 목록
	TArray<FPRTickOptimizationEntry> ActiveEntries;

	// 다음 평가 시작 시 활성 목록에 추가할 대상 목록
	TSet<TWeakObjectPtr<AActor>> PendingRegisterTargets;

	// 다음 평가 시작 시 활성 목록에서 제거할 대상 목록
	TSet<TWeakObjectPtr<AActor>> PendingUnregisterTargets;

	// 한 평가 주기 동안 재사용하는 플레이어 기준점 목록
	TArray<FPRTickOptimizationSource> CachedSources;

	// 설정 간격 평가를 구동하는 타이머 핸들
	FTimerHandle EvaluationTimerHandle;

	// 거리 평가 간격
	float EvaluationInterval = 0.25f;

	// 한 평가 호출에서 처리할 최대 대상 수. 0 이하는 전체 처리
	int32 MaxTargetsPerEvaluation = 0;

	// 프레임 분산 평가에서 다음으로 처리할 활성 목록 인덱스
	int32 NextEvaluationIndex = 0;
};

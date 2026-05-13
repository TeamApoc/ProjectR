// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProjectR/AI/PREnemyAITypes.h"
#include "PREnemyThreatComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPRTargetChangedSignature, AActor*, OldTarget, AActor*, NewTarget);

// 몬스터의 타겟 후보와 현재 공격 대상을 서버에서 관리하는 컴포넌트다.
// 1차 리팩터링에서는 기존 최고 Threat 선택 동작을 유지하되, Targeting 확장을 위한 후보/공격 커밋 상태를 함께 보관한다.
UCLASS(ClassGroup = (ProjectR), meta = (BlueprintSpawnableComponent))
class PROJECTR_API UPREnemyThreatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPREnemyThreatComponent();

	AActor* GetCurrentTarget() const { return CurrentTarget; }

	// 공격 커밋 중이면 고정된 공격 대상을, 아니면 현재 타겟을 반환한다.
	AActor* GetAttackTarget() const;

	// 현재 공격 커밋에 고정된 타겟을 반환한다.
	AActor* GetActiveAttackTarget() const { return AttackCommitState.ActiveAttackTarget; }

	// 현재 공격 커밋에 고정된 타겟 위치를 반환한다.
	const FVector& GetActiveAttackTargetLocation() const { return AttackCommitState.ActiveAttackTargetLocation; }

	// 공격 커밋으로 타겟 전환을 막고 있는지 확인한다.
	bool IsAttackCommitted() const { return AttackCommitState.bIsAttackCommitted; }

	// 현재 시점에 current_target을 즉시 교체할 수 있는지 확인한다.
	bool CanSwitchCurrentTarget() const;

	// 현재 후보 목록을 반환한다.
	const TArray<FPREnemyTargetCandidate>& GetTargetCandidates() const { return TargetCandidates; }

	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI")
	void AddThreat(AActor* Target, float Amount);

	// 피해를 준 플레이어를 공격 후보에 등록하고 현재 공격 대상 전환을 요청한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI")
	void AddDamageThreat(AActor* Target, float DamageAmount);

	// 지정한 후보를 현재 공격 대상으로 강제 선택한다. 공격 커밋 중이면 보류 후보로 저장된다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI")
	void ForceCurrentTarget(AActor* NewTarget);

	// Perception이 플레이어를 감지했을 때 공격 후보 상태를 갱신한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI")
	void UpdatePerceivedTarget(AActor* Target, FVector SensedLocation, bool bHasLOS);

	// Perception이 플레이어를 놓쳤을 때 후보 유지 상태를 갱신한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI")
	void MarkTargetPerceptionLost(AActor* LostTarget, FVector LastKnownLocation);

	// 후보 정보를 외부에서 읽을 수 있게 복사해 반환한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI")
	bool GetTargetCandidate(AActor* Target, FPREnemyTargetCandidate& OutCandidate) const;

	// 후보 유지 반경과 만료 시간을 기준으로 더 이상 유효하지 않은 후보를 정리한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI")
	void RefreshTargetCandidates();

	// 현재 공격 대상을 이번 공격의 고정 대상으로 잠근다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI")
	void BeginAttackCommit(AActor* InTarget, FVector InTargetLocation);

	// 공격 잠금을 해제하고 보류된 타겟 전환을 재평가한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI")
	void EndAttackCommit();

	// 사망/그로기처럼 공격이 강제로 끊길 때 잠금 상태를 즉시 정리한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI")
	void ForceClearAttackCommit();

	// 현재 타겟이 죽거나 유효하지 않게 되었을 때 다음 후보로 교체한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI")
	void InvalidateCurrentTarget();

	// 현재 타겟만 비워 수색/복귀 BT가 동작하게 한다. Threat 기록은 유지한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI")
	void ReleaseCurrentTargetForSearch(AActor* LostTarget);

	// Perception에서 타겟을 잃었다는 신호가 왔을 때 위협 목록을 정리한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI")
	void OnTargetLost(AActor* LostTarget);

public:
	UPROPERTY(BlueprintAssignable, Category = "ProjectR|AI")
	FPRTargetChangedSignature OnTargetChanged;

protected:
	bool IsValidThreatTarget(const AActor* Target) const;
	void ReevaluateTarget();
	void SetCurrentTarget(AActor* NewTarget, float CandidateScore = 0.0f);
	void CleanupInvalidEntries();
	// 10초 점수 창이 만료되었으면 피해 누적 점수를 초기화한다.
	bool ResetScoreWindowIfNeeded(float CurrentTime);
	// 후보의 피해/거리/LOS/현재 타겟 유지 점수를 다시 계산한다.
	float UpdateCandidateSelectionScore(FPREnemyTargetCandidate& Candidate) const;
	// 최종 선택 점수를 가중치로 사용해 다음 공격 대상을 확률 선택한다.
	bool SelectWeightedTarget(AActor*& OutTarget, float& OutScore) const;
	FPREnemyTargetCandidate* FindTargetCandidate(AActor* Target);
	const FPREnemyTargetCandidate* FindTargetCandidate(AActor* Target) const;
	FPREnemyTargetCandidate& FindOrAddTargetCandidate(AActor* Target, float CurrentTime);
	bool ShouldRemoveTargetCandidate(const FPREnemyTargetCandidate& Candidate, float CurrentTime) const;
	void QueuePendingTarget(AActor* NewTarget, float CandidateScore);
	void ClearPendingTarget();

protected:
	// 타겟 후보 유지와 선택 정책이다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|AI")
	FPREnemyTargetingConfig TargetingConfig;

	// 현재 공격 후보 목록이다.
	UPROPERTY()
	TArray<FPREnemyTargetCandidate> TargetCandidates;

	// BT/Ability가 참조하는 현재 공격 대상이다.
	UPROPERTY()
	TObjectPtr<AActor> CurrentTarget = nullptr;

	// 공격 하나가 진행 중일 때 타겟 변경을 보류하기 위한 상태다.
	UPROPERTY()
	FPREnemyAttackCommitState AttackCommitState;

	float LastSwitchTime = -1000.0f;

	float LastScoreWindowResetTime = -1000.0f;
};

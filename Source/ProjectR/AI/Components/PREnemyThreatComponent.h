// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProjectR/AI/PREnemyAITypes.h"
#include "PREnemyThreatComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPRTargetChangedSignature, AActor*, OldTarget, AActor*, NewTarget);

// 몬스터의 타겟 우선순위를 서버에서 관리하는 컴포넌트다.
// Perception이나 데미지 이벤트가 AddThreat를 호출하면 가장 위협이 높은 대상을 CurrentTarget으로 고른다.
UCLASS(ClassGroup = (ProjectR), meta = (BlueprintSpawnableComponent))
class PROJECTR_API UPREnemyThreatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPREnemyThreatComponent();

	AActor* GetCurrentTarget() const { return CurrentTarget; }

	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI")
	void AddThreat(AActor* Target, float Amount);

	// 현재 타겟이 죽거나 유효하지 않게 되었을 때 다음 후보로 교체한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI")
	void InvalidateCurrentTarget();

	// Perception에서 타겟을 잃었다는 신호가 왔을 때 위협 목록을 정리한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI")
	void OnTargetLost(AActor* LostTarget);

public:
	UPROPERTY(BlueprintAssignable, Category = "ProjectR|AI")
	FPRTargetChangedSignature OnTargetChanged;

protected:
	void ReevaluateTarget();
	void SetCurrentTarget(AActor* NewTarget);
	void CleanupInvalidEntries();

protected:
	// 새 타겟이 현재 타겟보다 이 비율 이상 위협적일 때만 전환한다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|AI")
	float SwitchRatio = 1.3f;

	// 타겟이 너무 자주 바뀌지 않도록 전환 후 잠깐 잠근다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|AI")
	float SwitchCooldown = 3.0f;

	// 이 시간 동안 갱신되지 않은 위협 항목은 잊어버린다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|AI")
	float ThreatForgetTime = 10.0f;

	UPROPERTY()
	TArray<FPRThreatEntry> ThreatList;

	// BT/Ability가 참조하는 현재 공격 대상이다.
	UPROPERTY()
	TObjectPtr<AActor> CurrentTarget = nullptr;

	float LastSwitchTime = -1000.0f;
};

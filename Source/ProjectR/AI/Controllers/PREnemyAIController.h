// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "ProjectR/AI/PREnemyAITypes.h"
#include "PREnemyAIController.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Hearing;
class UAISenseConfig_Sight;
class UBlackboardComponent;
class UBehaviorTree;
class UPRPerceptionConfig;
class UPREnemyCombatDataAsset;
class UPREnemyThreatComponent;
struct FPREnemyMovePresentationConfig;

// 공용 적 AIController
// Perception, Threat, Blackboard, 전투 표현 문맥 적용 담당
UCLASS()
class PROJECTR_API APREnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	APREnemyAIController();

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	// tactical_mode 갱신 및 전투 표현 규칙 적용
	void ApplyTacticalModeState(
		EPRTacticalMode NewMode,
		AActor* FocusTarget = nullptr,
		const FPREnemyMovePresentationConfig* OverridePresentationConfig = nullptr);

	// 전투 이동 표현 문맥 적용
	void ApplyCombatMovePresentationContext(AActor* FocusTarget, const FPREnemyMovePresentationConfig& PresentationConfig);

	// 전투 이동 표현 문맥 해제
	void ClearCombatMovePresentationContext(bool bClearGameplayFocus);

protected:
	/*~ AIPerception Interface ~*/

	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	/*~ Threat Interface ~*/

	UFUNCTION()
	void HandleThreatTargetChanged(AActor* OldTarget, AActor* NewTarget);

	// Perception 설정 적용
	void ApplyPerceptionConfig(const UPRPerceptionConfig* Config);

	// 현재 Blackboard tactical_mode 반환
	EPRTacticalMode GetBlackboardTacticalMode() const;

	// last_known_target_location 유효 여부 반환
	bool HasLastKnownTargetLocation() const;

	// Blackboard tactical_mode 값 기록
	void SetBlackboardTacticalMode(EPRTacticalMode NewMode);

	// 현재 소유 Pawn 기준 전투 데이터 자산 반환
	const UPREnemyCombatDataAsset* GetCurrentCombatDataAsset() const;

	// tactical_mode 규칙 또는 override 기반 전투 표현 적용 또는 해제
	void ApplyPresentationForTacticalMode(
		EPRTacticalMode NewMode,
		AActor* FocusTarget,
		const FPREnemyMovePresentationConfig* OverridePresentationConfig = nullptr);

	// 탐지 직후 시작 attack_pressure 적용
	void ApplyInitialAttackPressureOnAlert();

	// BehaviorTree / Blackboard 준비 및 실행
	void CacheBlackboardFromBehaviorTree(UBehaviorTree* BehaviorTreeAsset);

	// Threat 바인딩 해제
	void ClearThreatBinding();

protected:
	// AI Perception 컴포넌트
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|AI")
	TObjectPtr<UAIPerceptionComponent> EnemyPerceptionComponent;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

	UPROPERTY(Transient)
	TObjectPtr<UBlackboardComponent> CachedBlackboardComponent;

	UPROPERTY(Transient)
	TObjectPtr<UPREnemyThreatComponent> CachedThreatComponent;

	// 현재 타겟 Blackboard 키 이름
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|AI|Blackboard")
	FName CurrentTargetKey = TEXT("current_target");

	// 타겟 위치 Blackboard 키 이름
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|AI|Blackboard")
	FName TargetLocationKey = TEXT("target_location");

	// 마지막 목격 위치 Blackboard 키 이름
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|AI|Blackboard")
	FName LastKnownTargetLocationKey = TEXT("last_known_target_location");

	// 복귀 위치 Blackboard 키 이름
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|AI|Blackboard")
	FName HomeLocationKey = TEXT("home_location");

	// LOS Blackboard 키 이름
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|AI|Blackboard")
	FName HasLOSKey = TEXT("has_los");

	// tactical_mode Blackboard 키 이름
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|AI|Blackboard")
	FName TacticalModeKey = TEXT("tactical_mode");

	// attack_pressure Blackboard 키 이름
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|AI|Blackboard")
	FName AttackPressureKey = TEXT("attack_pressure");

	// 타겟 상실 시 처리 정책
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|AI|Perception")
	EPRTargetLostPolicy TargetLostPolicy = EPRTargetLostPolicy::ClearCurrentTarget;

	// 다음 타겟 해제 시 Investigate로 이어가기 위한 Alert 유지 플래그
	UPROPERTY(Transient)
	bool bPreserveAlertOnNextTargetClear = false;
};

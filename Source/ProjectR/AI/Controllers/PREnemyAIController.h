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
class UPREnemyThreatComponent;

// 모든 일반 몬스터가 사용하는 서버 권한 AIController다.
// Perception으로 타겟 후보를 수집하고, ThreatComponent가 고른 타겟을 Blackboard에 반영한다.
UCLASS()
class PROJECTR_API APREnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	APREnemyAIController();

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

protected:
	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	// ThreatComponent가 현재 타겟을 바꾸면 BT가 바로 따라갈 수 있도록 Blackboard를 갱신한다.
	UFUNCTION()
	void HandleThreatTargetChanged(AActor* OldTarget, AActor* NewTarget);

	// 몬스터 데이터 에셋 값을 실제 AIPerception 설정에 적용한다.
	void ApplyPerceptionConfig(const UPRPerceptionConfig* Config);
	void SetBlackboardTacticalMode(EPRTacticalMode NewMode);

	// BehaviorTree의 BlackboardAsset을 기준으로 BlackboardComponent를 준비하고 BT를 실행한다.
	void CacheBlackboardFromBehaviorTree(UBehaviorTree* BehaviorTreeAsset);
	void ClearThreatBinding();

protected:
	// Sight/Hearing Sense를 담는 실제 Perception 컴포넌트다.
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

	// 아래 키 이름들은 BT/Blackboard 에셋과 반드시 맞아야 한다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|AI|Blackboard")
	FName CurrentTargetKey = TEXT("current_target");

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|AI|Blackboard")
	FName TargetLocationKey = TEXT("target_location");

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|AI|Blackboard")
	FName LastKnownTargetLocationKey = TEXT("last_known_target_location");

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|AI|Blackboard")
	FName HomeLocationKey = TEXT("home_location");

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|AI|Blackboard")
	FName HasLOSKey = TEXT("has_los");

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|AI|Blackboard")
	FName TacticalModeKey = TEXT("tactical_mode");
};

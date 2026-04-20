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

	UFUNCTION()
	void HandleThreatTargetChanged(AActor* OldTarget, AActor* NewTarget);

	void ApplyPerceptionConfig(const UPRPerceptionConfig* Config);
	void SetBlackboardTacticalMode(EPRTacticalMode NewMode);
	void CacheBlackboardFromBehaviorTree(UBehaviorTree* BehaviorTreeAsset);
	void ClearThreatBinding();

protected:
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

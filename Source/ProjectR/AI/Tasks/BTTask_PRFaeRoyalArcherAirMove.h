// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "ProjectR/AI/PREnemyAITypes.h"
#include "BTTask_PRFaeRoyalArcherAirMove.generated.h"

class UPRFaeRoyalArcherCombatDataAsset;
class UBlackboardComponent;
class UBehaviorTreeComponent;
class AActor;
class APawn;
class ACharacter;

// Fae Royal Archer의 공중 이동 의도다.
UENUM(BlueprintType)
enum class EPRFaeRoyalArcherAirMoveMode : uint8
{
	CombatStrafe	UMETA(DisplayName = "Combat Strafe"),
	LosReposition	UMETA(DisplayName = "LOS Reposition"),
	CloseEvade		UMETA(DisplayName = "Close Evade"),
	SearchLastKnown UMETA(DisplayName = "Search Last Known"),
	PatrolHome		UMETA(DisplayName = "Patrol Home"),
	ReturnHome		UMETA(DisplayName = "Return Home")
};

// Fae Royal Archer 전용 공중 이동 Task다.
// NavMesh MoveTo 대신 Flying 이동 입력으로 타겟 기준 공중 위치를 유지한다.
UCLASS()
class PROJECTR_API UBTTask_PRFaeRoyalArcherAirMove : public UBTTaskNode
{
	GENERATED_BODY()

public:
	// 공중 이동 Task 기본 실행 속성을 초기화한다.
	UBTTask_PRFaeRoyalArcherAirMove();

	/*~ UBTTaskNode Interface ~*/
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual FString GetStaticDescription() const override;

protected:
	// Task 실행에 필요한 Blackboard, Pawn, DataAsset 참조를 찾는다.
	bool ResolveContext(
		UBehaviorTreeComponent& OwnerComp,
		UBlackboardComponent*& OutBlackboardComponent,
		APawn*& OutControlledPawn,
		AActor*& OutCurrentTarget,
		UPRFaeRoyalArcherCombatDataAsset*& OutCombatDataAsset) const;

	// 선택된 공중 이동 모드에 맞는 목표 위치를 계산한다.
	bool BuildAirMoveGoal(
		const APawn* ControlledPawn,
		const AActor* CurrentTarget,
		const UBlackboardComponent* BlackboardComponent,
		const UPRFaeRoyalArcherCombatDataAsset& CombatDataAsset,
		FVector& OutGoalLocation);

	// 계산된 목표 위치가 장애물과 LOS 조건을 만족하는지 확인한다.
	bool ValidateAirMoveGoal(
		const APawn* ControlledPawn,
		const AActor* CurrentTarget,
		const UPRFaeRoyalArcherCombatDataAsset& CombatDataAsset,
		const FVector& GoalLocation,
		bool bRequireLineOfSight) const;

	// 목표 위치 방향으로 Flying 이동 입력을 적용한다.
	void ApplyFlyingMovement(APawn* ControlledPawn, const UPRFaeRoyalArcherCombatDataAsset& CombatDataAsset, float DeltaSeconds) const;

	// 이동 종료 시 이동 입력과 표현 문맥을 정리한다.
	void FinishAirMove(UBehaviorTreeComponent& OwnerComp, EBTNodeResult::Type Result);

private:
	bool HasBlackboardKey(const UBlackboardComponent* BlackboardComponent, FName KeyName) const;
	bool DoesMoveModeRequireTarget() const;
	FVector ReadHomeLocation(const UBlackboardComponent* BlackboardComponent, const APawn* ControlledPawn) const;
	FVector ReadSearchAnchorLocation(const UBlackboardComponent* BlackboardComponent, const APawn* ControlledPawn) const;
	bool TryBuildAirCombatQueryGoal(const APawn* ControlledPawn, const AActor* CurrentTarget, const UPRFaeRoyalArcherCombatDataAsset& CombatDataAsset, bool bRequireLineOfSight, FVector& OutGoalLocation) const;
	bool TryBuildScoredAirCombatGoal(const APawn* ControlledPawn, const AActor* CurrentTarget, const UPRFaeRoyalArcherCombatDataAsset& CombatDataAsset, bool bRequireLineOfSight, FVector& OutGoalLocation) const;
	FVector BuildTargetRingGoal(const APawn* ControlledPawn, const AActor* CurrentTarget, const UPRFaeRoyalArcherCombatDataAsset& CombatDataAsset, float SignedArcDegrees, float DistanceOffset, float HeightOffset) const;
	FVector BuildCloseEvadeGoal(const APawn* ControlledPawn, const AActor* CurrentTarget, const UPRFaeRoyalArcherCombatDataAsset& CombatDataAsset) const;
	FVector BuildSearchLastKnownGoal(const UBlackboardComponent* BlackboardComponent, const APawn* ControlledPawn, const UPRFaeRoyalArcherCombatDataAsset& CombatDataAsset, float SignedArcDegrees) const;
	FVector BuildPatrolHomeGoal(const UBlackboardComponent* BlackboardComponent, const APawn* ControlledPawn, const UPRFaeRoyalArcherCombatDataAsset& CombatDataAsset, float SignedArcDegrees) const;
	FVector BuildReturnHomeGoal(const UBlackboardComponent* BlackboardComponent, const APawn* ControlledPawn, const UPRFaeRoyalArcherCombatDataAsset& CombatDataAsset) const;
	FVector ClampGoalAltitude(const FVector& GoalLocation, float ReferenceZ, const UPRFaeRoyalArcherCombatDataAsset& CombatDataAsset) const;
	float ScoreAirCombatGoal(const APawn* ControlledPawn, const AActor* CurrentTarget, const UPRFaeRoyalArcherCombatDataAsset& CombatDataAsset, const FVector& GoalLocation, bool bHasLineOfSight) const;
	bool HasLineOfSightFromGoal(const APawn* ControlledPawn, const AActor* CurrentTarget, const FVector& GoalLocation) const;
	bool IsDisabledByGameplayState(const APawn* ControlledPawn) const;
	void ApplyPresentationContext(UBehaviorTreeComponent& OwnerComp, AActor* CurrentTarget, const UPRFaeRoyalArcherCombatDataAsset& CombatDataAsset) const;
	void ClearPresentationContext(UBehaviorTreeComponent& OwnerComp) const;
	void CleanupAirMove(UBehaviorTreeComponent& OwnerComp, EBTNodeResult::Type Result) const;

protected:
	// 현재 타겟 Blackboard 키다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName CurrentTargetKey = TEXT("current_target");

	// 마지막으로 목격한 타겟 위치 Blackboard 키다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName LastKnownTargetLocationKey = TEXT("last_known_target_location");

	// 현재 타겟 위치 Blackboard 키다. 마지막 목격 위치가 없을 때 수색 기준점으로만 사용한다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName TargetLocationKey = TEXT("target_location");

	// 복귀 기준 위치 Blackboard 키다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName HomeLocationKey = TEXT("home_location");

	// 계산된 공중 이동 목표를 기록할 Blackboard 키다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName MoveGoalLocationKey = TEXT("move_goal_location");

	// tactical_mode Blackboard 키다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Blackboard")
	FName TacticalModeKey = TEXT("tactical_mode");

	// 궁수 전용 공중 전투 데이터 override다. 비어 있으면 Pawn의 CombatDataAsset을 사용한다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|RoyalArcher")
	TObjectPtr<UPRFaeRoyalArcherCombatDataAsset> CombatDataAssetOverride = nullptr;

	// 수행할 공중 이동 의도다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|RoyalArcher")
	EPRFaeRoyalArcherAirMoveMode MoveMode = EPRFaeRoyalArcherAirMoveMode::CombatStrafe;

	// 성공 시 tactical_mode를 기록할지 결정한다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|RoyalArcher")
	bool bSetTacticalModeOnStart = true;

	// 공중 이동 성공 시작 시 기록할 tactical_mode다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|RoyalArcher", meta = (EditCondition = "bSetTacticalModeOnStart"))
	EPRTacticalMode TacticalModeOnStart = EPRTacticalMode::Strafe;

	// 종료 시 즉시 이동 속도를 끊을지 결정한다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|RoyalArcher")
	bool bStopMovementOnFinish = true;

	// 이동 종료 시 전투 표현 문맥을 해제할지 결정한다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|RoyalArcher")
	bool bClearPresentationOnFinish = false;

	// 장애물 Sweep에 사용할 채널이다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Trace")
	TEnumAsByte<ECollisionChannel> ObstacleTraceChannel = ECC_WorldStatic;

	// LOS 확인에 사용할 채널이다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Trace")
	TEnumAsByte<ECollisionChannel> LOSCheckTraceChannel = ECC_Visibility;

private:
	FVector ActiveGoalLocation = FVector::ZeroVector;
	float EndTimeSeconds = 0.0f;
	float NextStrafeDirectionSign = 1.0f;
};

// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayAbilitySpec.h"
#include "ProjectR/AI/Data/PRFaerinCombatLoopDataAsset.h"
#include "PRFaerinCombatDirectorComponent.generated.h"

class AAIController;
class APREnemyBaseCharacter;
class UBehaviorTreeComponent;
class UPRAbilitySystemComponent;
class UPRPatternDataAsset;
struct FAbilityEndedData;
struct FPREnemyPatternQueryRuntime;

UENUM(BlueprintType)
enum class EPRFaerinCombatLoopState : uint8
{
	Idle				UMETA(DisplayName = "Idle"),
	SelectingPattern	UMETA(DisplayName = "Selecting Pattern"),
	WaitingPatternEnd	UMETA(DisplayName = "Waiting Pattern End"),
	PostPatternStrafe	UMETA(DisplayName = "Post Pattern Strafe")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPRFaerinLoopStepFinishedSignature, bool, bSucceeded);

// Faerin 전용 공격 루프를 PatternData, LoopData, AbilitySet 기반으로 실행하는 컴포넌트다.
UCLASS(ClassGroup = (ProjectR), meta = (BlueprintSpawnableComponent))
class PROJECTR_API UPRFaerinCombatDirectorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Faerin 루프 실행을 위한 기본값을 설정한다.
	UPRFaerinCombatDirectorComponent();

	/*~ UActorComponent Interface ~*/
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 현재 Blackboard 문맥 기준으로 다음 Faerin 공격 루프 한 단계를 실행한다.
	bool RunNextLoopStep(UBehaviorTreeComponent& OwnerComp);

	// 진행 중인 루프 단계를 중단하고 내부 상태를 정리한다.
	void CancelCurrentLoopStep();

	// 현재 루프 단계를 새로 시작할 수 있는지 확인한다.
	bool CanRunLoopStep() const;

	// 현재 루프 실행 상태를 반환한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|AI|Boss|Faerin")
	EPRFaerinCombatLoopState GetLoopState() const { return LoopState; }

	// 현재 체력 비율과 페이즈 설정을 기준으로 공격 후 횡이동 시간을 계산한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Faerin")
	float CalculateStrafeDuration() const;

protected:
	// Owner와 데이터 참조를 다시 확인하고 캐시한다.
	bool InitializeDirector();

	// 현재 BT 문맥에서 선택 가능한 패턴 후보를 고른다.
	bool SelectPatternPlan(UBehaviorTreeComponent& OwnerComp, FPRFaerinPatternPlan& OutPlan);

	// 선택된 패턴 Ability를 서버 ASC에서 실행한다.
	bool ActivatePatternAbility(const FPRFaerinPatternPlan& PatternPlan);

	// Ability 종료 델리게이트를 연결한다.
	void BindAbilityEndDelegate();

	// Ability 종료 델리게이트를 해제한다.
	void ClearAbilityEndDelegate();

	// 관찰 중인 Ability가 아직 활성 상태인지 확인한다.
	bool IsObservedAbilityActive() const;

	// 관찰 중인 Ability 종료 콜백을 처리한다.
	void HandleObservedAbilityEnded(const FAbilityEndedData& EndedData);

	// Ability 종료 이후 후속 루프 동작을 시작한다.
	void HandlePatternExecutionFinished(bool bSucceeded);

	// 공격 후 횡이동이 필요한지 판단한다.
	bool ShouldRunPostPatternStrafe(const FPRFaerinPatternPlan& PatternPlan, const FPRFaerinPhaseLoopConfig& PhaseConfig) const;

	// 원작형 공격 후 횡이동을 시작한다.
	bool StartPostPatternStrafe(const FPRFaerinPhaseLoopConfig& PhaseConfig);

	// 횡이동 목적지를 계산한다.
	bool ResolvePostPatternStrafeDestination(const FPRFaerinPhaseLoopConfig& PhaseConfig, FVector& OutDestination) const;

	// 횡이동 타이머가 끝났을 때 호출된다.
	void HandlePostPatternStrafeElapsed();

	// 전체 루프 단계 타임아웃이 발생했을 때 호출된다.
	void HandleLoopStepFailSafeElapsed();

	// 루프 단계를 완료하고 BT에 결과를 알린다.
	void FinishLoopStep(bool bSucceeded);

	// 다음 틱에 루프 완료를 예약한다.
	void QueueFinishLoopStep(bool bSucceeded);

	// 검증 문제를 로그로 출력한다.
	void LogValidationErrors(const TArray<FString>& Errors) const;

	// 현재 체력 비율을 ASC Attribute에서 계산한다.
	float ResolveHealthRatio() const;

	// Owner의 현재 보스 페이즈를 반환한다.
	EPRBossPhase ResolveCurrentPhase() const;

	// 패턴 선택 결과를 디버그 로그로 남긴다.
	void LogSelectedPattern(const FPRFaerinPatternPlan& PatternPlan, const FPREnemyPatternQueryRuntime& PatternRuntime) const;

private:
	// 현재 Owner의 AIController를 반환한다.
	AAIController* GetOwnerAIController() const;

public:
	UPROPERTY(BlueprintAssignable, Category = "ProjectR|AI|Boss|Faerin")
	FPRFaerinLoopStepFinishedSignature OnFaerinLoopStepFinished;

protected:
	// Faerin 원작형 루프 설정 데이터다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin")
	TObjectPtr<UPRFaerinCombatLoopDataAsset> LoopDataAsset;

	// BeginPlay 시 PatternData/LoopData 정합성을 1차 점검할지 결정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin")
	bool bValidateOnBeginPlay = true;

	// 한 루프 단계가 멈췄을 때 강제로 실패 처리할 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin", meta = (ClampMin = "0.0"))
	float LoopStepFailSafeSeconds = 12.0f;

	// 패턴 선택 시 사용할 카테고리 필터다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin")
	EPRPatternCategory PatternCategoryFilter = EPRPatternCategory::Any;

	// 패턴 선택 시 사용할 그룹 필터다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin", meta = (Categories = "Pattern.Boss.Faerin"))
	FGameplayTag PatternGroupFilter;

	// Blackboard 현재 타깃 키 이름이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Blackboard")
	FName CurrentTargetKey = TEXT("current_target");

	// Blackboard LOS 키 이름이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Blackboard")
	FName HasLOSKey = TEXT("has_los");

	// Blackboard 돌진 경로 키 이름이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Blackboard")
	FName ChargePathClearKey = TEXT("charge_path_clear");

	// Blackboard 전술 상태 키 이름이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Blackboard")
	FName TacticalModeKey = TEXT("tactical_mode");

	// Blackboard 공격 압박 키 이름이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Blackboard")
	FName AttackPressureKey = TEXT("attack_pressure");

private:
	UPROPERTY(Transient)
	TObjectPtr<APREnemyBaseCharacter> OwnerEnemy;

	UPROPERTY(Transient)
	TObjectPtr<UPRAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(Transient)
	TObjectPtr<UPRPatternDataAsset> PatternDataAsset;

	UPROPERTY(Transient)
	TObjectPtr<AActor> ActiveTarget;

	FPRFaerinPatternPlan ActivePatternPlan;
	FGameplayAbilitySpecHandle ActiveAbilityHandle;
	FDelegateHandle AbilityEndedDelegateHandle;
	FTimerHandle LoopStepFailSafeTimerHandle;
	FTimerHandle PostPatternStrafeTimerHandle;
	EPRFaerinCombatLoopState LoopState = EPRFaerinCombatLoopState::Idle;
	int32 NextStrafeDirectionSign = 1;
	bool bHasLoggedRuntimeValidationErrors = false;
	bool bHasLoggedInitializationError = false;
};

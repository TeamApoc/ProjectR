// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "ProjectR/AbilitySystem/Abilities/Boss/PRGameplayAbility_BossPatternBase.h"
#include "ProjectR/AI/Data/PREnemyCombatDataAsset.h"
#include "PRGameplayAbility_FaerinShiftSequence.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAnimMontage;
class UEnvQuery;
class UPRFaerinCharacterEventRouterComponent;

// Faerin Shift 패턴에서 타겟 이동 위치를 산출하는 방식이다.
UENUM(BlueprintType)
enum class EPRFaerinShiftDestinationMode : uint8
{
	EnvQuery				UMETA(DisplayName = "EnvQuery"),
	DirectionFromBossToTarget	UMETA(DisplayName = "Direction From Boss To Target")
};

// Faerin ShiftPlayerClose / ShiftPlayerAway를 하나의 공용 Ability로 처리한다.
// Close/Away 차이는 BP 파생 클래스의 이벤트명, EQS, 거리 설정으로만 나눈다.
UCLASS(Abstract)
class PROJECTR_API UPRGameplayAbility_FaerinShiftSequence : public UPRGameplayAbility_BossPatternBase
{
	GENERATED_BODY()

public:
	UPRGameplayAbility_FaerinShiftSequence();

	/*~ UGameplayAbility Interface ~*/
public:
	// Shift 몽타주를 재생하고 CharacterEvent 지점에서 타겟 위치를 이동시킨다.
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	// 몽타주 태스크와 CharacterEvent listener를 정리한다.
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	// Faerin CharacterEvent Router에 Shift 이벤트 listener를 등록한다.
	bool RegisterCharacterEventListener();

	// 등록한 Shift 이벤트 listener를 제거한다.
	void UnregisterCharacterEventListener();

	// ShiftPlayerClose / ShiftPlayerAway 이벤트를 받아 실제 타겟 이동을 수행한다.
	void HandleFaerinCharacterEvent(FName EventName);

	// 현재 설정에 맞춰 타겟 이동 목적지를 계산한다.
	bool ResolveShiftDestination(FVector& OutDestination) const;

	// EQS로 타겟 이동 목적지를 계산한다.
	bool ResolveQueryShiftDestination(FVector& OutDestination) const;

	// 보스와 타겟 방향 기준 거리로 타겟 이동 목적지를 계산한다.
	bool ResolveDirectionalShiftDestination(FVector& OutDestination) const;

	// 목적지를 지면에 투영한다.
	bool ProjectShiftDestinationToGround(const FVector& InDestination, FVector& OutDestination) const;

	// 현재 타겟을 계산된 목적지로 이동시킨다.
	bool ApplyShiftToTarget(const FVector& Destination);

	// Ability를 정상/취소 상태로 마무리한다.
	void FinishShiftSequence(bool bWasCancelled);

	// Shift 몽타주가 정상 종료됐을 때 Ability를 마무리한다.
	UFUNCTION()
	void HandleShiftMontageCompleted();

	// Shift 몽타주가 자연스럽게 블렌드아웃될 때 Ability를 마무리한다.
	UFUNCTION()
	void HandleShiftMontageBlendOut();

	// Shift 몽타주가 중단됐을 때 Ability를 취소한다.
	UFUNCTION()
	void HandleShiftMontageInterrupted();

protected:
	// Shift 패턴 표현 몽타주다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Shift|Animation")
	TObjectPtr<UAnimMontage> ShiftMontage;

	// Shift 몽타주 시작 섹션이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Shift|Animation")
	FName ShiftMontageStartSection = NAME_None;

	// Shift 몽타주 재생 속도다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Shift|Animation", meta = (ClampMin = "0.0"))
	float ShiftMontagePlayRate = 1.0f;

	// 이 Ability가 반응할 원작 CharacterEvent 이름이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Shift|Event")
	FName ShiftCharacterEventName = TEXT("ShiftPlayerClose");

	// 지정 이벤트가 오지 않았을 때 Ability를 실패로 끝낼지 여부다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Shift|Event")
	bool bRequireCharacterEventForShift = true;

	// 타겟 이동 목적지 산출 방식이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Shift|Destination")
	EPRFaerinShiftDestinationMode DestinationMode = EPRFaerinShiftDestinationMode::EnvQuery;

	// EQS 실패 시 방향 기반 목적지로 대체할지 여부다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Shift|Destination")
	bool bFallbackToDirectionalDestinationWhenQueryFails = true;

	// 방향 기반 목적지 계산 시 보스 기준 타겟 방향으로 떨어뜨릴 거리다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Shift|Destination", meta = (ClampMin = "0.0"))
	float DirectionalDistanceFromBoss = 550.0f;

	// Shift 목적지 선정 EQS다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Shift|EQS", meta = (EditCondition = "DestinationMode == EPRFaerinShiftDestinationMode::EnvQuery"))
	TObjectPtr<UEnvQuery> ShiftDestinationQuery;

	// EQS 실행 방식이다. 후보 랜덤 선택을 쓰면 내부 유틸에서 AllMatching으로 보정한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Shift|EQS", meta = (EditCondition = "DestinationMode == EPRFaerinShiftDestinationMode::EnvQuery"))
	TEnumAsByte<EEnvQueryRunMode::Type> ShiftQueryRunMode = EEnvQueryRunMode::SingleResult;

	// EQS 후보 선택 방식이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Shift|EQS", meta = (EditCondition = "DestinationMode == EPRFaerinShiftDestinationMode::EnvQuery"))
	EPREnemyQueryCandidateSelectionMode CandidateSelectionMode = EPREnemyQueryCandidateSelectionMode::BestScore;

	// EQS Named Float Param 목록이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Shift|EQS", meta = (EditCondition = "DestinationMode == EPRFaerinShiftDestinationMode::EnvQuery"))
	TArray<FPREnemyEQSFloatParam> FloatParams;

	// 상위 후보 최대 선택 개수다. 0 이하면 개수 제한이 없다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Shift|EQS", meta = (EditCondition = "DestinationMode == EPRFaerinShiftDestinationMode::EnvQuery", ClampMin = "0"))
	int32 TopCandidateCount = 0;

	// 최고 점수 대비 유지할 최소 점수 비율이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Shift|EQS", meta = (EditCondition = "DestinationMode == EPRFaerinShiftDestinationMode::EnvQuery", ClampMin = "0.0", ClampMax = "1.0"))
	float TopScoreCandidateRatio = 0.9f;

	// 목적지를 지면으로 보정할지 여부다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Shift|Ground")
	bool bProjectDestinationToGround = true;

	// 지면 투영 실패 시 Shift 자체를 실패 처리할지 여부다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Shift|Ground", meta = (EditCondition = "bProjectDestinationToGround"))
	bool bRequireGroundProjection = false;

	// 지면 트레이스 시작 높이 오프셋이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Shift|Ground", meta = (EditCondition = "bProjectDestinationToGround", ClampMin = "0.0"))
	float GroundTraceUpOffset = 500.0f;

	// 지면 트레이스 하향 길이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Shift|Ground", meta = (EditCondition = "bProjectDestinationToGround", ClampMin = "0.0"))
	float GroundTraceDownOffset = 1500.0f;

	// 지면 투영 결과에 추가할 위치 오프셋이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Shift|Ground", meta = (EditCondition = "bProjectDestinationToGround"))
	FVector GroundLocationOffset = FVector::ZeroVector;

	// 지면 투영에 사용할 트레이스 채널이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Shift|Ground", meta = (EditCondition = "bProjectDestinationToGround"))
	TEnumAsByte<ECollisionChannel> GroundTraceChannel = ECC_Visibility;

	// 타겟 이동 시 Sweep을 사용할지 여부다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Shift|Target")
	bool bSweepTargetMove = false;

	// Shift 후 타겟 CharacterMovement 속도를 정지할지 여부다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Shift|Target")
	bool bStopTargetMovementAfterShift = true;

	// Shift 후 타겟이 보스를 바라보도록 회전을 보정할지 여부다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Shift|Target")
	bool bFaceBossAfterShift = true;

	// Shift 목적지 디버그 표시 여부다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Shift|Debug")
	bool bDrawDebug = false;

	// Shift 목적지 디버그 표시 시간이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Shift|Debug", meta = (EditCondition = "bDrawDebug", ClampMin = "0.0"))
	float DebugDrawDuration = 2.0f;

private:
	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> ActiveShiftMontageTask;

	UPROPERTY(Transient)
	TObjectPtr<UPRFaerinCharacterEventRouterComponent> ActiveEventRouter;

	bool bShiftApplied = false;
	bool bShiftSequenceFinished = false;
};

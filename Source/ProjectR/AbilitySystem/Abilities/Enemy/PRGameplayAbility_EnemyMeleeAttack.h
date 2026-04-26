// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "ProjectR/AbilitySystem/Abilities/Enemy/PRGameplayAbility_EnemyBase.h"
#include "PRGameplayAbility_EnemyMeleeAttack.generated.h"

class ACharacter;
class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;
class UAnimMontage;

// 근접 공격 공통 Ability
// 몽타주 재생, 애님 노티파이 기반 히트 윈도우, 구체 Sweep 판정을 담당한다.
UCLASS(Abstract)
class PROJECTR_API UPRGameplayAbility_EnemyMeleeAttack : public UPRGameplayAbility_EnemyBase
{
	GENERATED_BODY()

public:
	// 근접 공격 공통 Ability를 초기화한다.
	UPRGameplayAbility_EnemyMeleeAttack();

	/*~ UGameplayAbility Interface ~*/
public:
	// 근접 공격 Ability를 활성화하고 몽타주/노티파이 대기를 시작한다.
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	// 근접 공격 Ability를 종료하고 타이머/태스크를 정리한다.
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

public:
	// 애님 노티파이에서 수동으로 근접 판정을 요청한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Combat")
	void TriggerMeleeHitFromAnimation();

protected:
	// 단발 근접 히트 이벤트를 처리한다.
	UFUNCTION()
	void HandleMeleeHitGameplayEvent(FGameplayEventData Payload);

	// 근접 판정 구간 시작 이벤트를 처리한다.
	UFUNCTION()
	void HandleMeleeHitWindowBeginGameplayEvent(FGameplayEventData Payload);

	// 근접 판정 구간 갱신 이벤트를 처리한다.
	UFUNCTION()
	void HandleMeleeHitWindowTickGameplayEvent(FGameplayEventData Payload);

	// 근접 판정 구간 종료 이벤트를 처리한다.
	UFUNCTION()
	void HandleMeleeHitWindowEndGameplayEvent(FGameplayEventData Payload);

	// 공격 몽타주 정상 종료를 처리한다.
	UFUNCTION()
	void HandleAttackMontageCompleted();

	// 공격 몽타주 중단을 처리한다.
	UFUNCTION()
	void HandleAttackMontageInterrupted();

	// 실제 Sweep 판정과 피해 적용을 수행한다.
	virtual void ExecuteMeleeHit();

	// 단발 히트 입력을 한 번만 처리한다.
	void TriggerMeleeHitOnce();

	// 공격 종료 처리를 수행한다.
	void FinishMeleeAttack();

	// 히트 윈도우 상태를 시작한다.
	void BeginMeleeHitWindow();

	// 히트 윈도우 상태를 갱신한다.
	void UpdateMeleeHitWindow();

	// 히트 윈도우 상태를 종료한다.
	void EndMeleeHitWindow();

	// 루트모션 없는 공격의 전진 이동을 적용한다.
	void ApplyForwardLunge(ACharacter* SourceCharacter) const;

	// 현재 후보 액터가 피해 대상인지 확인한다.
	bool ShouldDamageActor(const AActor* CandidateActor) const;

	// 현재 타겟 방향으로 액터 회전을 갱신한다.
	void RefreshAttackFacing(bool bApplyActorRotation) const;

	// ThreatComponent 기준 현재 타겟을 반환한다.
	AActor* GetCurrentThreatTarget() const;

	// 현재 공격 판정 중심점을 계산한다.
	bool GetCurrentAttackTracePoint(FVector& OutTracePoint) const;

	// 두 지점 사이 Sweep 판정과 피해 적용을 수행한다.
	void ExecuteMeleeTrace(const FVector& TraceStart, const FVector& TraceEnd);

protected:
	// BTTask 활성화용 Ability 식별 태그
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat")
	FGameplayTag AbilityTag;

	// 체력 피해량
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat", meta = (ClampMin = "0.0"))
	float Damage = 10.0f;

	// 그로기 게이지 피해량
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat", meta = (ClampMin = "0.0"))
	float GroggyDamage = 10.0f;

	// 타이머 기반 판정 fallback용 선딜 시간
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat", meta = (ClampMin = "0.0"))
	float WindupTime = 0.25f;

	// 판정 이후 Ability 종료까지의 후딜 시간
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat", meta = (ClampMin = "0.0"))
	float RecoveryTime = 0.35f;

	// 공격 모션 몽타주
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Animation")
	TObjectPtr<UAnimMontage> AttackMontage;

	// 몽타주 시작 섹션
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Animation")
	FName MontageStartSection = NAME_None;

	// 몽타주 재생 속도
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Animation", meta = (ClampMin = "0.0"))
	float MontagePlayRate = 1.0f;

	// 몽타주 길이를 기준으로 종료 타이밍을 잡을지 여부
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Animation")
	bool bUseMontageDurationForFinish = true;

	// 애님 노티파이 기반 근접 판정을 사용할지 여부
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Animation")
	bool bUseAnimationNotifyForHit = true;

	// 한 Ability 내에서 여러 번의 히트를 허용할지 여부
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Animation")
	bool bAllowMultipleMeleeHits = false;

	// 몽타주 재생 중에도 타이머 fallback 판정을 허용할지 여부
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Animation")
	bool bAllowTimedHitFallbackWhenMontagePlays = false;

	// 소켓 미사용 시 정면 Sweep 거리
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat", meta = (ClampMin = "0.0"))
	float AttackRange = 220.0f;

	// Sweep 구체 반경
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat", meta = (ClampMin = "0.0"))
	float AttackRadius = 75.0f;

	// 공격 판정 기준 소켓 또는 본 이름
	// None이면 캐릭터 정면 Sweep을 사용한다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat")
	FName AttackTraceSourceName = NAME_None;

	// 소켓 또는 본 기준 근접 판정 중심점 오프셋
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat")
	FVector AttackTraceSourceOffset = FVector::ZeroVector;

	// 소켓 미지정 시 정면 Sweep 시작점 높이 오프셋
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat")
	float TraceHeightOffset = 50.0f;

	// 근접 Sweep 충돌 채널
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Pawn;

	// 현재 ThreatTarget만 피해 대상으로 제한할지 여부
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat")
	bool bOnlyDamageThreatTarget = true;

	// Ability 시작 시 타겟 방향으로 회전할지 여부
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Facing")
	bool bFaceTargetOnAbilityStart = true;

	// 히트 시점마다 타겟 방향으로 회전할지 여부
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Facing")
	bool bFaceTargetOnMeleeHit = true;

	// 루트모션 없는 공격에서 전진 이동을 사용할지 여부
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Movement")
	bool bUseForwardLunge = false;

	// 전진 이동 거리
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Movement", meta = (ClampMin = "0.0"))
	float LungeDistance = 0.0f;

	// 근접 판정 디버그 표시 여부
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Debug")
	bool bDrawDebug = false;

private:
	FTimerHandle HitTimerHandle;
	FTimerHandle FinishTimerHandle;

	// 한 번의 판정 구간에서 이미 피해를 준 액터 목록
	TSet<TWeakObjectPtr<AActor>> DamagedActors;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> ActiveMontageTask;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> ActiveMeleeHitEventTask;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> ActiveMeleeHitWindowBeginEventTask;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> ActiveMeleeHitWindowTickEventTask;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> ActiveMeleeHitWindowEndEventTask;

	// 중복 종료 방지 플래그
	bool bMeleeAttackFinished = false;

	// 단발 히트 처리 여부
	bool bMeleeHitTriggered = false;

	// 히트 윈도우 진행 여부
	bool bMeleeHitWindowActive = false;

	// 직전 히트 윈도우 위치 보유 여부
	bool bHasPreviousMeleeWindowTracePoint = false;

	// 직전 히트 윈도우 위치
	FVector PreviousMeleeWindowTracePoint = FVector::ZeroVector;
};

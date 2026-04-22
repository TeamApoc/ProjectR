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

// 적 근접 공격 공통 Ability
// 몽타주 재생, AnimNotify 기반 타격, 서버 Sweep 판정, 데미지 적용 처리
UCLASS(Abstract)
class PROJECTR_API UPRGameplayAbility_EnemyMeleeAttack : public UPRGameplayAbility_EnemyBase
{
	GENERATED_BODY()

public:
	UPRGameplayAbility_EnemyMeleeAttack();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

	// Notify 이벤트 테스트 또는 BP 직접 연결용 백업 진입점
	// 기본 흐름은 GameplayEvent와 WaitGameplayEvent 사용
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Combat")
	void TriggerMeleeHitFromAnimation();

protected:
	UFUNCTION()
	void HandleMeleeHitGameplayEvent(FGameplayEventData Payload);

	UFUNCTION()
	void HandleAttackMontageCompleted();

	UFUNCTION()
	void HandleAttackMontageInterrupted();

	// 실제 Sweep 판정과 데미지 적용
	virtual void ExecuteMeleeHit();

	// 단일 공격 내 중복 타격 방지 래퍼
	void TriggerMeleeHitOnce();
	void FinishMeleeAttack();

	// 루트모션 없는 특수 공격용 백업 전진 이동
	void ApplyForwardLunge(ACharacter* SourceCharacter) const;

	// ASC 없음 또는 현재 ThreatTarget 외 대상 제외
	bool ShouldDamageActor(const AActor* CandidateActor) const;

protected:
	// BTTask 활성화용 Ability 식별 태그
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat")
	FGameplayTag AbilityTag;

	// Health 적용 피해량
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat", meta = (ClampMin = "0.0"))
	float Damage = 10.0f;

	// GroggyGauge 적용 피해량
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat", meta = (ClampMin = "0.0"))
	float GroggyDamage = 10.0f;

	// 타이머 기반 판정의 공격 시작 후 타격 지연 시간
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat", meta = (ClampMin = "0.0"))
	float WindupTime = 0.25f;

	// 공격 판정 이후 Ability 종료 전 후딜 시간
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat", meta = (ClampMin = "0.0"))
	float RecoveryTime = 0.35f;

	// 공격 모션 재생 몽타주
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Animation")
	TObjectPtr<UAnimMontage> AttackMontage;

	// 몽타주 시작 섹션
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Animation")
	FName MontageStartSection = NAME_None;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Animation", meta = (ClampMin = "0.0"))
	float MontagePlayRate = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Animation")
	bool bUseMontageDurationForFinish = true;

	// PR Enemy Melee Hit Notify 기반 타격 프레임 사용 여부
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Animation")
	bool bUseAnimationNotifyForHit = true;

	// Notify 누락 시 WindupTime 백업 판정 허용 여부
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Animation")
	bool bAllowTimedHitFallbackWhenMontagePlays = false;

	// 공격 시작 위치 기준 정면 Sweep 거리
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat", meta = (ClampMin = "0.0"))
	float AttackRange = 220.0f;

	// Sweep 구체 반경
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat", meta = (ClampMin = "0.0"))
	float AttackRadius = 75.0f;

	// 몸통 높이 판정용 Z 오프셋
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat")
	float TraceHeightOffset = 50.0f;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Pawn;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat")
	bool bOnlyDamageThreatTarget = true;

	// 루트모션 없는 공격 전용 옵션
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Movement")
	bool bUseForwardLunge = false;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Movement", meta = (ClampMin = "0.0"))
	float LungeDistance = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Debug")
	bool bDrawDebug = false;

private:
	FTimerHandle HitTimerHandle;
	FTimerHandle FinishTimerHandle;

	// 단일 공격 내 Actor별 중복 타격 기록
	TSet<TWeakObjectPtr<AActor>> DamagedActors;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> ActiveMontageTask;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> ActiveMeleeHitEventTask;

	// 몽타주 종료/타이머/취소 중복 종료 방지
	bool bMeleeAttackFinished = false;
	bool bMeleeHitTriggered = false;
};

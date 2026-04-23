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
// 몽타주 재생, AnimNotify 기반 타격 이벤트 수신, 서버 Sweep 판정 담당
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

	// AnimNotify에서 직접 호출하는 타격 진입 함수
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Combat")
	void TriggerMeleeHitFromAnimation();

protected:
	UFUNCTION()
	void HandleMeleeHitGameplayEvent(FGameplayEventData Payload);

	UFUNCTION()
	void HandleAttackMontageCompleted();

	UFUNCTION()
	void HandleAttackMontageInterrupted();

	// 실제 Sweep 판정과 피해 적용
	virtual void ExecuteMeleeHit();

	// 단일 공격 내 중복 타격 방지 처리
	void TriggerMeleeHitOnce();
	void FinishMeleeAttack();

	// 루트 모션이 없는 공격용 전진 보정
	void ApplyForwardLunge(ACharacter* SourceCharacter) const;

	// 유효한 피격 대상인지 확인
	bool ShouldDamageActor(const AActor* CandidateActor) const;

	// 현재 대상 방향으로 회전 및 워프 타겟 갱신
	void RefreshAttackFacing(bool bApplyActorRotation) const;

	// ThreatComponent 기준 현재 공격 대상 반환
	AActor* GetCurrentThreatTarget() const;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat")
	FGameplayTag AbilityTag; // BTTask 호출용 Ability 식별 태그

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat", meta = (ClampMin = "0.0"))
	float Damage = 10.0f; // 체력 피해량

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat", meta = (ClampMin = "0.0"))
	float GroggyDamage = 10.0f; // 그로기 피해량

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat", meta = (ClampMin = "0.0"))
	float WindupTime = 0.25f; // 타격 판정 전 준비 시간

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat", meta = (ClampMin = "0.0"))
	float RecoveryTime = 0.35f; // 판정 이후 종료 전 대기 시간

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Animation")
	TObjectPtr<UAnimMontage> AttackMontage; // 공격 재생용 몽타주

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Animation")
	FName MontageStartSection = NAME_None; // 몽타주 시작 섹션

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Animation", meta = (ClampMin = "0.0"))
	float MontagePlayRate = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Animation")
	bool bUseMontageDurationForFinish = true;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Animation")
	bool bUseAnimationNotifyForHit = true; // PR Enemy Melee Hit Notify 기반 타격 이벤트 사용 여부

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Animation")
	bool bAllowMultipleMeleeHits = false; // 하나의 Ability 안에서 여러 번의 타격 Notify 허용 여부

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Animation")
	bool bAllowTimedHitFallbackWhenMontagePlays = false; // Notify 누락 시 WindupTime 기반 보조 판정 사용 여부

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat", meta = (ClampMin = "0.0"))
	float AttackRange = 220.0f; // 정면 Sweep 거리

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat", meta = (ClampMin = "0.0"))
	float AttackRadius = 75.0f; // Sweep 구체 반경

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat")
	float TraceHeightOffset = 50.0f; // 판정 높이 보정값

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Pawn;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat")
	bool bOnlyDamageThreatTarget = true;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Facing")
	bool bFaceTargetOnAbilityStart = true; // Ability 시작 시 대상 방향 즉시 회전 여부

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Facing")
	bool bFaceTargetOnMeleeHit = true; // 타격 Notify 시점 대상 방향 즉시 회전 여부

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Facing")
	bool bUpdateMotionWarpTarget = true; // Motion Warping 타겟 자동 갱신 여부

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Facing")
	FName MotionWarpTargetName = TEXT("AttackTarget"); // Motion Warping Notify State에서 참조하는 타겟 이름

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Facing")
	bool bUseTargetLocationForMotionWarping = false; // true면 타겟 위치를 워프 위치로 직접 사용

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Movement")
	bool bUseForwardLunge = false; // 루트 모션이 없는 공격 전용 전진 옵션

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Movement", meta = (ClampMin = "0.0"))
	float LungeDistance = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Debug")
	bool bDrawDebug = false;

private:
	FTimerHandle HitTimerHandle;
	FTimerHandle FinishTimerHandle;

	TSet<TWeakObjectPtr<AActor>> DamagedActors; // 단일 공격 내 Actor별 중복 타격 기록

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> ActiveMontageTask;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> ActiveMeleeHitEventTask;

	bool bMeleeAttackFinished = false; // 몽타주 종료, 타이머 종료, 취소 중복 처리 방지
	bool bMeleeHitTriggered = false;
};

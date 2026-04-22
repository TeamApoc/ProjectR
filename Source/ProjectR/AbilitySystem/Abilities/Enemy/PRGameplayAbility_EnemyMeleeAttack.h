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

// 적 근접 공격의 공통 Ability다.
// 몽타주 재생, AnimNotify 기반 타격, 서버 Sweep 판정, 데미지 적용까지 한 번에 처리한다.
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

	// Notify 이벤트를 테스트하거나 BP에서 직접 연결해야 할 때 쓰는 백업 진입점이다.
	// 기본 흐름은 GameplayEvent를 WaitGameplayEvent로 받는다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Combat")
	void TriggerMeleeHitFromAnimation();

protected:
	UFUNCTION()
	void HandleMeleeHitGameplayEvent(FGameplayEventData Payload);

	UFUNCTION()
	void HandleAttackMontageCompleted();

	UFUNCTION()
	void HandleAttackMontageInterrupted();

	// 실제 Sweep 판정과 데미지 적용을 수행한다.
	void ExecuteMeleeHit();

	// 한 공격에서 타격이 두 번 이상 들어가지 않도록 보호하는 래퍼다.
	void TriggerMeleeHitOnce();
	void FinishMeleeAttack();

	// 루트모션이 없는 특수 공격을 위한 백업 전진 이동이다. 루트모션 공격에서는 꺼둔다.
	void ApplyForwardLunge(ACharacter* SourceCharacter) const;

	// ASC가 없는 Actor나 현재 ThreatTarget이 아닌 대상을 제외한다.
	bool ShouldDamageActor(const AActor* CandidateActor) const;

protected:
	// BTTask가 활성화할 때 사용하는 이 Ability의 식별 태그다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat")
	FGameplayTag AbilityTag;

	// Health에 들어갈 피해량이다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat", meta = (ClampMin = "0.0"))
	float Damage = 10.0f;

	// GroggyGauge에 들어갈 피해량이다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat", meta = (ClampMin = "0.0"))
	float GroggyDamage = 10.0f;

	// 타이머 기반 판정을 사용할 때 공격 시작 후 타격까지 걸리는 시간이다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat", meta = (ClampMin = "0.0"))
	float WindupTime = 0.25f;

	// 공격 판정 이후 Ability를 끝내기 전까지의 후딜 시간이다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat", meta = (ClampMin = "0.0"))
	float RecoveryTime = 0.35f;

	// 공격 모션으로 재생할 몽타주다. 지정되면 기본적으로 AnimNotify가 타격 타이밍을 결정한다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Animation")
	TObjectPtr<UAnimMontage> AttackMontage;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Animation", meta = (ClampMin = "0.0"))
	float MontagePlayRate = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Animation")
	bool bUseMontageDurationForFinish = true;

	// true면 몽타주 안의 PR Enemy Melee Hit Notify가 타격 프레임을 결정한다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Animation")
	bool bUseAnimationNotifyForHit = true;

	// Notify가 없을 때도 WindupTime 판정을 백업으로 쓸지 여부다. 기본은 중복 판정 방지를 위해 false다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Animation")
	bool bAllowTimedHitFallbackWhenMontagePlays = false;

	// 공격 시작 위치에서 정면으로 Sweep할 거리다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat", meta = (ClampMin = "0.0"))
	float AttackRange = 220.0f;

	// Sweep 구체 반경이다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat", meta = (ClampMin = "0.0"))
	float AttackRadius = 75.0f;

	// 캐릭터 발밑이 아니라 몸통 높이에서 판정하기 위한 Z 오프셋이다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat")
	float TraceHeightOffset = 50.0f;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Pawn;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat")
	bool bOnlyDamageThreatTarget = true;

	// 루트모션이 없는 공격에서만 사용하는 옵션이다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Movement")
	bool bUseForwardLunge = false;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Movement", meta = (ClampMin = "0.0"))
	float LungeDistance = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Debug")
	bool bDrawDebug = false;

private:
	FTimerHandle HitTimerHandle;
	FTimerHandle FinishTimerHandle;

	// 한 번의 공격 안에서 같은 Actor를 여러 번 맞히지 않도록 기록한다.
	TSet<TWeakObjectPtr<AActor>> DamagedActors;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> ActiveMontageTask;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> ActiveMeleeHitEventTask;

	// 몽타주 종료/타이머/취소가 겹쳐도 EndAbility가 한 번만 호출되도록 한다.
	bool bMeleeAttackFinished = false;
	bool bMeleeHitTriggered = false;
};

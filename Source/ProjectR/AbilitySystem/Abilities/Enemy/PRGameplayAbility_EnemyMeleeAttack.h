// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Abilities/Enemy/PRGameplayAbility_EnemyBase.h"
#include "PRGameplayAbility_EnemyMeleeAttack.generated.h"

class ACharacter;
class UAbilityTask_PlayMontageAndWait;
class UAnimMontage;

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

	UFUNCTION(BlueprintCallable, Category = "ProjectR|Combat")
	void TriggerMeleeHitFromAnimation();

protected:
	UFUNCTION()
	void HandleAttackMontageCompleted();

	UFUNCTION()
	void HandleAttackMontageInterrupted();

	void ExecuteMeleeHit();
	void TriggerMeleeHitOnce();
	void FinishMeleeAttack();
	void ApplyForwardLunge(ACharacter* SourceCharacter) const;
	bool ShouldDamageActor(const AActor* CandidateActor) const;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat")
	FGameplayTag AbilityTag;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat", meta = (ClampMin = "0.0"))
	float Damage = 10.0f;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat", meta = (ClampMin = "0.0"))
	float GroggyDamage = 10.0f;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat", meta = (ClampMin = "0.0"))
	float WindupTime = 0.25f;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat", meta = (ClampMin = "0.0"))
	float RecoveryTime = 0.35f;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Animation")
	TObjectPtr<UAnimMontage> AttackMontage;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Animation", meta = (ClampMin = "0.0"))
	float MontagePlayRate = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Animation")
	bool bUseMontageDurationForFinish = true;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Animation")
	bool bUseAnimationNotifyForHit = true;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Animation")
	bool bAllowTimedHitFallbackWhenMontagePlays = false;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat", meta = (ClampMin = "0.0"))
	float AttackRange = 220.0f;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat", meta = (ClampMin = "0.0"))
	float AttackRadius = 75.0f;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat")
	float TraceHeightOffset = 50.0f;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Pawn;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat")
	bool bOnlyDamageThreatTarget = true;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Movement")
	bool bUseForwardLunge = false;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Movement", meta = (ClampMin = "0.0"))
	float LungeDistance = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Debug")
	bool bDrawDebug = false;

private:
	FTimerHandle HitTimerHandle;
	FTimerHandle FinishTimerHandle;
	TSet<TWeakObjectPtr<AActor>> DamagedActors;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> ActiveMontageTask;

	bool bMeleeAttackFinished = false;
	bool bMeleeHitTriggered = false;
};

// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "ProjectR/AbilitySystem/Abilities/Boss/PRGameplayAbility_FaerinStagedMontagePattern.h"
#include "PRGameplayAbility_FaerinThrowSequence.generated.h"

class APRProjectileBase;
class UGameplayEffect;

// Faerin 원작 Throw 계열의 몽타주 실행과 투사체 발사를 처리하는 패턴 Ability다.
UCLASS()
class PROJECTR_API UPRGameplayAbility_FaerinThrowSequence : public UPRGameplayAbility_FaerinStagedMontagePattern
{
	GENERATED_BODY()

public:
	UPRGameplayAbility_FaerinThrowSequence();

	/*~ UGameplayAbility Interface ~*/
public:
	// Throw 실행마다 투사체 발사 상태를 초기화한 뒤 staged montage 패턴을 시작한다.
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	// Throw 종료 시 예약된 투사체 발사 타이머를 정리한다.
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	// 지정된 Throw 스테이지가 시작되면 투사체 발사를 예약한다.
	virtual void NativeOnStageStarted(const FPRFaerinStagedMontageStage& Stage) override;

	// Throw 투사체 발사를 예약한다.
	void ScheduleThrowProjectile();

	// Throw 투사체를 실제로 생성하고 초기 속도와 피해 Spec을 주입한다.
	void SpawnThrowProjectile();

	// Throw 투사체 스폰 transform을 계산한다.
	bool BuildThrowProjectileSpawnTransform(FTransform& OutTransform) const;

	// Throw 투사체 조준 방향을 계산한다.
	FVector CalculateThrowAimDirection(const FVector& SpawnLocation) const;

	// Throw 투사체에 주입할 enemy damage Spec을 만든다.
	FGameplayEffectSpecHandle BuildThrowProjectileEffectSpec() const;

	// Throw 스폰 기준 위치를 보스 mesh socket 또는 로컬 오프셋으로 계산한다.
	FVector ResolveThrowSpawnLocation() const;

protected:
	// 투사체를 발사할 스테이지 이름이다. 비워 두면 첫 스테이지 시작 시 발사 예약을 건다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Throw")
	FName ThrowStageName = TEXT("Throw");

	// Throw 몽타주 시작 후 투사체가 손을 떠나는 지연 시간이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Throw", meta = (ClampMin = "0.0"))
	float ThrowReleaseDelay = 1.0f;

	// Throw가 생성할 투사체 클래스다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Throw|Projectile")
	TSubclassOf<APRProjectileBase> ThrowProjectileClass;

	// Throw 투사체 피해에 사용할 GameplayEffect다. 비워 두면 AbilitySystemRegistry의 Enemy damage GE를 사용한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Throw|Projectile")
	TSubclassOf<UGameplayEffect> ProjectileDamageEffectClass;

	// 투사체를 스폰할 보스 mesh socket 이름이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Throw|Projectile")
	FName ProjectileSpawnSocketName = TEXT("WP_Blade");

	// socket이 없거나 비어 있을 때 사용할 보스 로컬 스폰 오프셋이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Throw|Projectile")
	FVector ProjectileSpawnLocalOffset = FVector(120.0f, 0.0f, 120.0f);

	// 투사체 스폰 회전에 더할 오프셋이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Throw|Projectile")
	FRotator ProjectileRotationOffset = FRotator::ZeroRotator;

	// Throw 투사체 속도 override다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Throw|Projectile", meta = (ClampMin = "0.0"))
	float ProjectileSpeedOverride = 3500.0f;

	// 타겟 이동 속도를 얼마나 선행 조준에 반영할지 나타내는 비율 값이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Throw|Projectile", meta = (ClampMin = "0.0"))
	float ProjectileTargetLead = 5.0f;

	// Throw 투사체가 homing으로 타겟을 추적할지 여부다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Throw|Projectile")
	bool bUseTrackingProjectile = false;

	// homing 사용 시 적용할 가속도다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Throw|Projectile",
		meta = (EditCondition = "bUseTrackingProjectile", ClampMin = "0.0"))
	float ProjectileHomingAcceleration = 4000.0f;

	// Enemy AttackPower에 곱할 Throw 투사체 피해 배율이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Throw|Projectile", meta = (ClampMin = "0.0"))
	float ProjectileDamageMultiplier = 1.0f;

	// Throw 투사체가 부여할 강인도 피해다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|Throw|Projectile", meta = (ClampMin = "0.0"))
	float ProjectilePoiseDamage = 10.0f;

private:
	FTimerHandle ThrowReleaseTimerHandle;
	uint32 NextThrowProjectileId = 1;
	bool bThrowProjectileSpawned = false;
};

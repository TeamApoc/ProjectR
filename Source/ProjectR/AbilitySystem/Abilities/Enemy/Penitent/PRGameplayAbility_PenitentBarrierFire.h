// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Abilities/Enemy/PRGameplayAbility_EnemyBase.h"
#include "PRGameplayAbility_PenitentBarrierFire.generated.h"

class UGameplayEffect;

// Penitent 배리어 발사 Ability
UCLASS()
class PROJECTR_API UPRGameplayAbility_PenitentBarrierFire : public UPRGameplayAbility_EnemyBase
{
	GENERATED_BODY()

public:
	// Ability 태그 초기화
	UPRGameplayAbility_PenitentBarrierFire();

	/*~ UGameplayAbility Interface ~*/
	// 배리어 발사 가능 여부 확인
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags,
		const FGameplayTagContainer* TargetTags,
		FGameplayTagContainer* OptionalRelevantTags) const override;

	// 배리어 발사 실행
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	// 발사 피해 GE Override
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Penitent|Barrier")
	TSubclassOf<UGameplayEffect> BarrierDamageEffectClass;

	// 체력 피해 배율
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Penitent|Barrier", meta = (ClampMin = "0.0"))
	float DamageMultiplier = 1.0f;

	// 고정 강인도 피해
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Penitent|Barrier", meta = (ClampMin = "0.0"))
	float PoiseDamage = 12.0f;

	// 배리어 발사 피해 Spec 생성
	FGameplayEffectSpecHandle BuildBarrierDamageEffectSpec() const;
};

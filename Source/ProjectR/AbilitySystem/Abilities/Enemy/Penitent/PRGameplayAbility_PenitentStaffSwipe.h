// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Abilities/Enemy/PRGameplayAbility_EnemyMeleeAttack.h"
#include "PRGameplayAbility_PenitentStaffSwipe.generated.h"

class UAnimMontage;

// Penitent 지팡이 휘두르기 Ability
UCLASS()
class PROJECTR_API UPRGameplayAbility_PenitentStaffSwipe : public UPRGameplayAbility_EnemyMeleeAttack
{
	GENERATED_BODY()

public:
	// Ability 태그와 기본 확률 초기화
	UPRGameplayAbility_PenitentStaffSwipe();

	/*~ UGameplayAbility Interface ~*/
	// 1타 또는 2타 몽타주 선택 후 근접 공격 실행
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	// 1타 휘두르기 몽타주
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Penitent|Animation")
	TObjectPtr<UAnimMontage> OneHitAttackMontage;

	// 2타 휘두르기 몽타주
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Penitent|Animation")
	TObjectPtr<UAnimMontage> TwoHitAttackMontage;

	// 2타 휘두르기 선택 확률
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Penitent|Combat", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TwoHitChance = 0.25f;
};

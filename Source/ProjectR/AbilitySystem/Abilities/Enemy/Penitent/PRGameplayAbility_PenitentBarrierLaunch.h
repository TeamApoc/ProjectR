// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Abilities/Enemy/PRGameplayAbility_EnemyBase.h"
#include "PRGameplayAbility_PenitentBarrierLaunch.generated.h"

// Penitent 배리어 발사 Ability
UCLASS()
class PROJECTR_API UPRGameplayAbility_PenitentBarrierLaunch : public UPRGameplayAbility_EnemyBase
{
	GENERATED_BODY()

public:
	// Ability 태그 초기화
	UPRGameplayAbility_PenitentBarrierLaunch();

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
	// 발사 속도
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Penitent|Barrier", meta = (ClampMin = "0.0"))
	float LaunchSpeed = 1800.0f;
};

// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/PRGameplayAbility.h"
#include "PRGA_SwapWeapon.generated.h"

/**
 * 주무기와 보조무기 슬롯의 활성 상태를 토글하는 어빌리티.
 * 현재 활성 슬롯의 반대 슬롯에 무기 Item이 있으면 활성 슬롯을 그쪽으로 전환.
 */
UCLASS()
class PROJECTR_API UPRGA_SwapWeapon : public UPRGameplayAbility
{
	GENERATED_BODY()

public:
	UPRGA_SwapWeapon();

	/*~ UGameplayAbility Interface ~*/
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags,
		const FGameplayTagContainer* TargetTags,
		FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
};

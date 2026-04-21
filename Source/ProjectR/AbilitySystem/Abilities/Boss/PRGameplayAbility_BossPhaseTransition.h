// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Abilities/Enemy/PRGameplayAbility_EnemyBase.h"
#include "ProjectR/AI/PREnemyAITypes.h"
#include "PRGameplayAbility_BossPhaseTransition.generated.h"

UCLASS(Abstract)
class PROJECTR_API UPRGameplayAbility_BossPhaseTransition : public UPRGameplayAbility_EnemyBase
{
	GENERATED_BODY()

public:
	UPRGameplayAbility_BossPhaseTransition();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|AI|Boss")
	EPRFaerinPhase TargetPhase = EPRFaerinPhase::Opening;
};

// Copyright ProjectR. All Rights Reserved.

#include "PRGameplayEffect_Damage.h"

#include "ProjectR/AbilitySystem/Executions/PRDamageExecutionCalculation.h"

UPRGameplayEffect_Damage::UPRGameplayEffect_Damage()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	// 데미지는 지속 효과가 아니라 즉시 계산이므로 ExecutionCalculation 하나만 연결한다.
	FGameplayEffectExecutionDefinition ExecutionDefinition;
	ExecutionDefinition.CalculationClass = UPRDamageExecutionCalculation::StaticClass();
	Executions.Add(ExecutionDefinition);
}

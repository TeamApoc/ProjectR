// Copyright ProjectR. All Rights Reserved.

#include "PRGameplayEffect_Damage.h"

#include "ProjectR/AbilitySystem/Executions/PRDamageExecutionCalculation.h"

UPRGameplayEffect_Damage::UPRGameplayEffect_Damage()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayEffectExecutionDefinition ExecutionDefinition;
	ExecutionDefinition.CalculationClass = UPRDamageExecutionCalculation::StaticClass();
	Executions.Add(ExecutionDefinition);
}

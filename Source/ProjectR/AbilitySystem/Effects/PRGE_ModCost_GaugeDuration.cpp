// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRGE_ModCost_GaugeDuration.h"

#include "ProjectR/AbilitySystem/Executions/PRModCostExecCalc_GaugeDuration.h"
#include "ProjectR/Combat/PRCombatGameplayTags.h"

UPRGE_ModCost_GaugeDuration::UPRGE_ModCost_GaugeDuration()
{
	// 지속시간형 비용 수명
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	FSetByCallerFloat DurationSetByCaller;
	DurationSetByCaller.DataTag = PRCombatGameplayTags::SetByCaller_ModDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(DurationSetByCaller);

	Period = FScalableFloat(0.1f);
	bExecutePeriodicEffectOnApplication = false;

	FGameplayEffectExecutionDefinition ExecutionDefinition;
	ExecutionDefinition.CalculationClass = UPRModCostExecCalc_GaugeDuration::StaticClass();
	Executions.Add(ExecutionDefinition);
}

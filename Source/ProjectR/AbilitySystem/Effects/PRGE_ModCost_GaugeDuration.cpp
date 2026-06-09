// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRGE_ModCost_GaugeDuration.h"

#include "ProjectR/AbilitySystem/Executions/PRModCostExecCalc_GaugeDuration.h"

UPRGE_ModCost_GaugeDuration::UPRGE_ModCost_GaugeDuration()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	Period = FScalableFloat(0.1f);
	bExecutePeriodicEffectOnApplication = false;

	FGameplayEffectExecutionDefinition ExecutionDefinition;
	ExecutionDefinition.CalculationClass = UPRModCostExecCalc_GaugeDuration::StaticClass();
	Executions.Add(ExecutionDefinition);
}

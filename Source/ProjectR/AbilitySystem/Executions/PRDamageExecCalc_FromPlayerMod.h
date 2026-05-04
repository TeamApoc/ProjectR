// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PRDamageExecCalcBase.h"
#include "PRDamageExecCalc_FromPlayerMod.generated.h"

// 플레이어 모드 스킬 발신 데미지 ExecCalc.
UCLASS()
class PROJECTR_API UPRDamageExecCalc_FromPlayerMod : public UPRDamageExecCalcBase
{
	GENERATED_BODY()

public:
	UPRDamageExecCalc_FromPlayerMod();

	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
		FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};

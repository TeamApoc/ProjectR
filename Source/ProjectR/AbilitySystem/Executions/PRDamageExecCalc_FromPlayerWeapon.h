// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PRDamageExecCalcBase.h"
#include "PRDamageExecCalc_FromPlayerWeapon.generated.h"

// 플레이어 총기 데미지 ExecCalc.
UCLASS()
class PROJECTR_API UPRDamageExecCalc_FromPlayerWeapon : public UPRDamageExecCalcBase
{
	GENERATED_BODY()

public:
	UPRDamageExecCalc_FromPlayerWeapon();

	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
		FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};

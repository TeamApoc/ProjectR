// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "PRDamageExecutionCalculation.generated.h"

// 공용 Damage GE가 호출하는 실행 계산식이다.
// SetByCaller 피해량과 HitResult 부위 정보를 읽어 Health/GroggyGauge를 실제로 감소시킨다.
UCLASS()
class PROJECTR_API UPRDamageExecutionCalculation : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UPRDamageExecutionCalculation();

	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
		FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};

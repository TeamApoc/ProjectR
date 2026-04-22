// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "PRGameplayEffect_Damage.generated.h"

// 피해 적용에 사용하는 공용 Instant GameplayEffect다.
// 실제 수치 계산은 PRDamageExecutionCalculation에서 처리한다.
UCLASS()
class PROJECTR_API UPRGameplayEffect_Damage : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UPRGameplayEffect_Damage();
};

// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Abilities/Boss/PRGameplayAbility_FaerinShiftSequence.h"
#include "PRGameplayAbility_FaerinShiftPlayerCloseSequence.generated.h"

// Faerin 원작 ShiftPlayerClose 패턴의 기본값을 고정하는 전용 Ability다.
UCLASS(Abstract)
class PROJECTR_API UPRGameplayAbility_FaerinShiftPlayerCloseSequence : public UPRGameplayAbility_FaerinShiftSequence
{
	GENERATED_BODY()

public:
	UPRGameplayAbility_FaerinShiftPlayerCloseSequence();
};

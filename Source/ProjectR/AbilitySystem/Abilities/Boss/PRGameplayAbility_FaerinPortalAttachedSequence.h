// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Abilities/Boss/PRGameplayAbility_BossPortalSequence.h"
#include "PRGameplayAbility_FaerinPortalAttachedSequence.generated.h"

// Faerin 2페이즈 부착형 좌/우 포탈 패턴의 C++ 부모 Ability다.
UCLASS()
class PROJECTR_API UPRGameplayAbility_FaerinPortalAttachedSequence : public UPRGameplayAbility_BossPortalSequence
{
	GENERATED_BODY()

public:
	UPRGameplayAbility_FaerinPortalAttachedSequence();
};


// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Abilities/Boss/PRGameplayAbility_BossPortalSequence.h"
#include "PRGameplayAbility_FaerinPortalMissileSequence.generated.h"

// Faerin 1페이즈 공중 포털 1개 설치 패턴의 전용 C++ 부모 Ability다.
UCLASS()
class PROJECTR_API UPRGameplayAbility_FaerinPortalMissileSequence : public UPRGameplayAbility_BossPortalSequence
{
	GENERATED_BODY()

public:
	UPRGameplayAbility_FaerinPortalMissileSequence();
};

// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PRGA_Mod.h"
#include "PRGA_Mod_HasDuration.generated.h"

// 지속시간형 비용 GE를 적용한 뒤 즉시 종료되는 모드 어빌리티
UCLASS(Abstract)
class PROJECTR_API UPRGA_Mod_HasDuration : public UPRGA_Mod
{
	GENERATED_BODY()

public:
	UPRGA_Mod_HasDuration();

public:
	/*~ UGameplayAbility Interface ~*/
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	// 지속시간형 어빌리티 시작 처리
	UFUNCTION(BlueprintNativeEvent, Category = "ProjectR|Mod|Duration")
	void OnDurationStarted();
	virtual void OnDurationStarted_Implementation();
};

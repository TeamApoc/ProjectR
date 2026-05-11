// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PRGA_Fire.h"
#include "PRGA_FireSingle.generated.h"

/**
 * 단발 사격 어빌리티. 1회 발사 후 즉시 EndAbility. 쿨다운(FireInterval) 동안 재활성화 차단
 */
UCLASS()
class PROJECTR_API UPRGA_FireSingle : public UPRGA_Fire
{
	GENERATED_BODY()

public:
	UPRGA_FireSingle();

public:
	/*~ UGameplayAbility Interface ~*/
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	// true이면 무기 데이터의 FireInterval 대신 FireIntervalOverride 사용
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Fire|Single")
	bool bOverrideFireInterval = false;

	// Override가 true일 때 사용할 발사 간격(초)
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Fire|Single")
	float FireIntervalOverride = 0.5f;
};

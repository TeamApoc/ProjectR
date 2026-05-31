// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "UObject/Interface.h"
#include "PRDirectDamageReceiverInterface.generated.h"

UINTERFACE(MinimalAPI)
class UPRDirectDamageReceiverInterface : public UInterface
{
	GENERATED_BODY()
};

// ASC를 소유하지 않는 전투 오브젝트가 무기/모드 피해 Spec을 직접 받을 때 사용하는 인터페이스다.
class PROJECTR_API IPRDirectDamageReceiverInterface
{
	GENERATED_BODY()

public:
	// 서버 권위에서 전달된 피해 Spec과 HitResult를 오브젝트 자체 체력 처리로 변환한다.
	virtual bool ApplyDirectDamageFromSpec(const FGameplayEffectSpec& DamageSpec, const FHitResult& HitResult) = 0;
};

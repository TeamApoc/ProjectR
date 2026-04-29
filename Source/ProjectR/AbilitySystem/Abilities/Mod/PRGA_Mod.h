// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/PRGameplayAbility.h"
#include "PRGA_Mod.generated.h"

// 플레이어 모드 스킬 어빌리티 베이스다.
// 데미지와 그로기 데미지는 SetByCaller로 전달하며, DamageGE_FromMod ExecCalc를 사용한다.
UCLASS(Abstract)
class PROJECTR_API UPRGA_Mod : public UPRGameplayAbility
{
	GENERATED_BODY()

public:
	UPRGA_Mod();

protected:
	// 모드 스킬 데미지 GE를 타겟에 적용한다. Damage와 GroggyDamage를 SetByCaller로 전달
	void ApplyDamage(AActor* TargetActor, float Damage, float GroggyDamage = 0.0f, const FHitResult* HitResult = nullptr);
};

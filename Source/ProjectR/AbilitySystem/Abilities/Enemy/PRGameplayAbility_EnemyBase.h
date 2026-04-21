// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/PRGameplayAbility.h"
#include "ProjectR/Combat/PRCombatTypes.h"
#include "PRGameplayAbility_EnemyBase.generated.h"

class APREnemyBaseCharacter;
class UGameplayEffect;

// 적 Ability의 공통 베이스다.
// 서버 실행 정책과 공용 데미지 적용 함수를 한 곳에 모아 패턴 Ability들이 같은 흐름을 타게 한다.
UCLASS(Abstract)
class PROJECTR_API UPRGameplayAbility_EnemyBase : public UPRGameplayAbility
{
	GENERATED_BODY()

public:
	UPRGameplayAbility_EnemyBase();

protected:
	// FPRDamageContext를 공용 Damage GE로 변환해 타겟 ASC에 적용한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Combat")
	bool ApplyDamageContext(FPRDamageContext DamageContext) const;

	// Enemy 전용 컴포넌트/데이터에 접근해야 할 때 사용하는 Avatar 캐스팅 헬퍼다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|Combat")
	APREnemyBaseCharacter* GetEnemyAvatarCharacter() const;

protected:
	// 기본값은 PRGameplayEffect_Damage이며, 몬스터별 특수 피해 GE가 필요하면 에디터에서 교체한다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Combat")
	TSubclassOf<UGameplayEffect> DamageEffectClass;
};

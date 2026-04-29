// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ProjectR/Combat/PRCombatTypes.h"
#include "ProjectR/Combat/PRCombatInterface.h"
#include "PRCombatStatics.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;


// 전투/피해 처리에서 공통으로 쓰는 헬퍼 함수 모음이다.
UCLASS()
class PROJECTR_API UPRCombatStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// IAbilitySystemInterface를 구현한 Actor에서 ASC를 찾는다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|Combat")
	static UAbilitySystemComponent* FindAbilitySystemComponent(const AActor* Actor);

	// 액터의 진영을 반환한다. IPRCombatInterface 미구현 시 Neutral 반환
	UFUNCTION(BlueprintPure, Category = "ProjectR|Combat")
	static EPRTeam GetActorTeam(const AActor* Actor);

	// 두 액터가 같은 진영인지 판정한다. 한쪽이라도 Neutral이면 false 반환
	UFUNCTION(BlueprintPure, Category = "ProjectR|Combat")
	static bool IsFriendly(const AActor* SourceActor, const AActor* TargetActor);

	// 부위 보정·프렌들리 감쇠를 적용한 최종 데미지·그로기 데미지를 계산한다.
	static FPRDamageOutputs ComputeDamage(const FPRDamageInputs& Inputs, const FHitResult& HitResult, const AActor* TargetActor);

	// 공격력(BaseDamage)에 비례하여 기본 그로기 피해량을 산출한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|Combat")
	static float CalculateBaseGroggyDamage(float BaseDamage);
};

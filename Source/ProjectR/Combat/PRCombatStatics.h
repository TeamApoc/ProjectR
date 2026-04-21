// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ProjectR/Combat/PRCombatTypes.h"
#include "PRCombatStatics.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
class UPrimitiveComponent;

// 전투/피해 처리에서 공통으로 쓰는 헬퍼 함수 모음이다.
// Ability는 이 클래스를 통해 ASC 탐색, 부위 판정, Damage GE 적용을 통일한다.
UCLASS()
class PROJECTR_API UPRCombatStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// IAbilitySystemInterface를 구현한 Actor에서 ASC를 찾는다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|Combat")
	static UAbilitySystemComponent* FindAbilitySystemComponent(const AActor* Actor);

	// HitResult의 ComponentTag를 해석해 Armor/Weakpoint 정보를 만든다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|Combat")
	static FPRDamageRegionInfo ResolveDamageRegion(const FHitResult& HitResult, const AActor* TargetActor);

	// 보스 코어 약점이 현재 열려 있는지 확인한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|Combat")
	static bool IsCoreWeakpointOpen(const AActor* TargetActor);

	// FPRDamageContext를 GameplayEffect Spec으로 변환해 타겟 ASC에 적용한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Combat")
	static bool ApplyDamageEffect(const FPRDamageContext& DamageContext,
		TSubclassOf<UGameplayEffect> DamageEffectClass);

private:
	static bool FindTaggedRegion(const UPrimitiveComponent* HitComponent, const TCHAR* Prefix, FName& OutRegionTag);
	static FPRDamageRegionInfo ResolveDamageRegionInternal(const FHitResult& HitResult,
		const UAbilitySystemComponent* TargetASC);
};

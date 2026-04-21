// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ProjectR/Combat/PRCombatTypes.h"
#include "PRCombatStatics.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
class UPrimitiveComponent;

UCLASS()
class PROJECTR_API UPRCombatStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "ProjectR|Combat")
	static UAbilitySystemComponent* FindAbilitySystemComponent(const AActor* Actor);

	UFUNCTION(BlueprintPure, Category = "ProjectR|Combat")
	static FPRDamageRegionInfo ResolveDamageRegion(const FHitResult& HitResult, const AActor* TargetActor);

	UFUNCTION(BlueprintPure, Category = "ProjectR|Combat")
	static bool IsCoreWeakpointOpen(const AActor* TargetActor);

	UFUNCTION(BlueprintCallable, Category = "ProjectR|Combat")
	static bool ApplyDamageEffect(const FPRDamageContext& DamageContext,
		TSubclassOf<UGameplayEffect> DamageEffectClass);

private:
	static bool FindTaggedRegion(const UPrimitiveComponent* HitComponent, const TCHAR* Prefix, FName& OutRegionTag);
	static FPRDamageRegionInfo ResolveDamageRegionInternal(const FHitResult& HitResult,
		const UAbilitySystemComponent* TargetASC);
};

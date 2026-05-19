// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PRGA_Mod.h"
#include "PRGA_Mod_GrantAbility.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTR_API UPRGA_Mod_GrantAbility : public UPRGA_Mod
{
	GENERATED_BODY()
	
public:
	UPRGA_Mod_GrantAbility();
	
	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
protected:
	void WaitModStackExhausted(EPRWeaponSlotType WeaponSlotType);
	
	UFUNCTION()
	void OnInputPressed(float TimeWaited);
	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UGameplayAbility> AbilityToGrant;
	
private:
	FGameplayAbilitySpecHandle GrantedSpecHandle;
	FGameplayAttribute CostAttribute;
	FDelegateHandle CostExhaustedHandle;
};

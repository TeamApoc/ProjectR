// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/PRGameplayAbility.h"
#include "PRGA_Interact.generated.h"

class UAbilityTask_WaitInputRelease;
class UPRInteractorComponent;
/**
 * 
 */
UCLASS()
class PROJECTR_API UPRGA_Interact : public UPRGameplayAbility
{
	GENERATED_BODY()
	
public:
	UPRGA_Interact();
	
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	
protected:
	UFUNCTION(BlueprintCallable)
	UPRInteractorComponent* GetInteractorComponent(AController* InController) const;
	
	void BindToSustained(UPRInteractorComponent* Interactor);
	void BindToHoldFinished(UPRInteractorComponent* Interactor);
	void WaitHoldRelease(UPRInteractorComponent* Interactor);
	
private:
	UFUNCTION()
	void OnInputReleased(float TimeHeld);
	
private:
	mutable TWeakObjectPtr<UPRInteractorComponent> CachedInteractorComponent;
	
	UPROPERTY()
	UAbilityTask_WaitInputRelease* WaitHoldReleaseTask = nullptr;
};

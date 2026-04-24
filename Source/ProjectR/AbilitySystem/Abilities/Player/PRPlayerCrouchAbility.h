// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/PRGameplayAbility.h"
#include "PRPlayerCrouchAbility.generated.h"

class UPRCameraModifier;

/**
 * 캐릭터의 앉기/일어서기를 제어하는 어빌리티
 */
UCLASS()
class PROJECTR_API UPRPlayerCrouchAbility : public UPRGameplayAbility
{
	GENERATED_BODY()
	
public:
	UPRPlayerCrouchAbility();

	/*~ UGameplayAbility Interface ~*/
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle SpecHandle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle SpecHandle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
override;
	
protected:
	UFUNCTION(Server, Reliable)
	void Server_SendCrouchLocation(FVector_NetQuantize100 ClientLocation);
	
private:
	/** 실제 앉기 로직 집행 */
	void ExecuteCrouch();

	UFUNCTION()
	void OnCrouchInputPressed(float TimeWaited);
	
	/** 활성화된 카메라 모디파이어 (구르기 종료 시 비활성화하기 위함) */
	UPROPERTY()
	TObjectPtr<UPRCameraModifier> ActiveCameraModifier;
	
private:
	/** 서버 데이터 수신 여부 */
	bool bServerReceivedLocation = false;
	FVector_NetQuantize100 SavedLocation;
};

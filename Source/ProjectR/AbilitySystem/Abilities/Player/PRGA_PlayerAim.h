// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/PRGameplayAbility.h"
#include "PRGA_PlayerAim.generated.h"

class UPRCrosshairConfig;

UCLASS()
class PROJECTR_API UPRGA_PlayerAim : public UPRGameplayAbility
{
	GENERATED_BODY()
	
public:
	UPRGA_PlayerAim();

	/*~ UGameplayAbility Interface ~*/
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	
protected:
	/** 조준 시 변경할 시야각(FOV) */
	UPROPERTY(EditDefaultsOnly, Category = "PR|Aim|Camera")
	float AimFOV = 50.0f;

	/** 조준 시 변경할 카메라 거리 */
	UPROPERTY(EditDefaultsOnly, Category = "PR|Aim|Camera")
	float AimTargetArmLength = 100.0f;

	/** 조준 시 변경할 회전축(TargetOffset) 높이 */
	UPROPERTY(EditDefaultsOnly, Category = "PR|Aim|Camera")
	FVector AimTargetOffset = FVector(0.0f, 0.0f, 75.0f);

	/** 조준 시 변경할 어깨 오프셋(SocketOffset) */
	UPROPERTY(EditDefaultsOnly, Category = "PR|Aim|Camera")
	FVector AimSocketOffset = FVector(0.0f, 40.0f, 0.0f);

	/** 26.04.26, Yuchan, 임시: 조준 시 사용할 크로스헤어 Config. 추후 WeaponManager 가 무기별로 관리 예정 */
	UPROPERTY(EditDefaultsOnly, Category = "PR|Aim|Crosshair")
	TObjectPtr<UPRCrosshairConfig> AimCrosshairConfig;
};

// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/PRGameplayAbility.h"
#include "PRGA_PlayerAim.generated.h"

/**
 * TODO: 임시로 AIM 몽타쥬를 넣고, 기본적으로 플레이어에게 스킬을 넣어두었으나, 무기를 들었을때 맞는 애니메이션, Armed태그를 가지고 있을때 스킬 활성화 등으로 발전해야함
 */
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
	// 임시 구현 몽타쥬
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PR|Aim")
	TObjectPtr<UAnimMontage> AimMontage;
	
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
};

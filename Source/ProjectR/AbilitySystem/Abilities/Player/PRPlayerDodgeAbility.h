// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/PRGameplayAbility.h"
#include "PRPlayerDodgeAbility.generated.h"

class UPRCameraModifier;
/**
 * 플레이어 구르기(회피) 어빌리티
 * 8방향 이동 입력을 기반으로 회피 방향을 결정하며, 특정 구간에 무적 판정을 부여합니다.
 */
UCLASS()
class PROJECTR_API UPRPlayerDodgeAbility : public UPRGameplayAbility
{
	GENERATED_BODY()

public:
    UPRPlayerDodgeAbility();

    /*~ UGameplayAbility Interface ~*/
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
    /*~ [서버 전용] 클라이언트로부터 방향데이터를 받는 RPC~*/
    UFUNCTION(Server, Reliable)
    void Server_SendDodgeDirection(FVector_NetQuantize Direction);
    
private:
    /** 몽타주 종료/취소 시 호출될 콜백 */
    UFUNCTION()
    void OnMontageCompleted();

    /** 무적 프레임 시작 시 호출될 콜백 */
    UFUNCTION()
    void StartIFrame();

    /** 무적 프레임 종료 시 호출될 콜백 */
    UFUNCTION()
    void EndIFrame();
    
    /** 실제 구르기 로직 집행 (클라/서버 공용) */
    void ExecuteDodge(FVector Direction);
    
protected:
    /** 구르기 애니메이션 몽타주 */
    UPROPERTY(EditDefaultsOnly, Category = "PR|Dodge")
    TObjectPtr<UAnimMontage> DodgeMontage;

    /** 무적 상태를 부여할 GameplayEffect 클래스 (State.Invulnerable 태그 포함) */
    UPROPERTY(EditDefaultsOnly, Category = "PR|Dodge")
    TSubclassOf<UGameplayEffect> InvulnerableEffectClass;

    /** 무적 프레임 시작 딜레이 (기본 0.1초) */
    UPROPERTY(EditDefaultsOnly, Category = "PR|Dodge")
    float IFrameStartDelay = 0.1f;

    /** 무적 프레임 지속 시간 (기본 0.35초 = 0.45 - 0.1) */
    UPROPERTY(EditDefaultsOnly, Category = "PR|Dodge")
    float IFrameDuration = 0.35f;

private:
    /** 활성화된 카메라 모디파이어 (구르기 종료 시 비활성화하기 위함) */
    UPROPERTY()
    TObjectPtr<UPRCameraModifier> ActiveCameraModifier;
    
    /** 적용된 무적 효과 핸들 */
    FActiveGameplayEffectHandle InvulnerableEffectHandle;
    
    /** 서버가 데이터를 받았는지 확인하기 위한 플래그 */
    bool bServerReceivedDirection = false;
    FVector SavedDirection = FVector::ZeroVector;
};

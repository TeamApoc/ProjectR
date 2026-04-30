// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/Animation/PRAnimationTypes.h"
#include "ProjectR/AbilitySystem/PRGameplayAbility.h"
#include "PRPlayerDodgeAbility.generated.h"

class UPRCameraModifier;
class UPRAnimInstance;
class ACharacter;
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
    void Server_SendDodgeDirection(FVector_NetQuantize Direction, bool bHasMovementInput);
    
private:
    /** 회피 애니메이션 종료 시 호출될 콜백 */
    UFUNCTION()
    void HandleDodgeAnimationFinished();

    /** 회피 최대 안전 시간이 끝났을 때 현재 실행과 일치하면 회피를 종료한다 */
    void HandleDodgeSafetyTimeout(int32 FinishedDodgeExecutionId);

    /** 무적 프레임 시작 타이머가 끝났을 때 현재 실행과 일치하면 무적을 시작한다 */
    void HandleIFrameStartTimer(int32 FinishedDodgeExecutionId);

    /** 무적 프레임 종료 타이머가 끝났을 때 현재 실행과 일치하면 무적을 종료한다 */
    void HandleIFrameEndTimer(int32 FinishedDodgeExecutionId);

    /** 무적 프레임을 즉시 적용한다 */
    void StartIFrame();

    /** 무적 프레임 종료 시 호출될 콜백 */
    void EndIFrame();

    /** 이전 회피 실행에서 남은 타이머를 정리한다 */
    void ClearDodgeTimers();
    
    /** 실제 구르기 로직 집행 (클라/서버 공용) */
    void ExecuteDodge(FVector Direction, bool bHasMovementInput);

    /** 현재 캐릭터 AnimInstance의 회피 종료 이벤트를 구독한다 */
    void BindDodgeAnimationFinished(ACharacter* Character);
    
protected:
    /** 무적 상태를 부여할 GameplayEffect 클래스 (State.Invulnerable 태그 포함) */
    UPROPERTY(EditDefaultsOnly, Category = "PR|Dodge")
    TSubclassOf<UGameplayEffect> InvulnerableEffectClass;

    /** 무적 프레임 시작 딜레이 (기본 0.1초) */
    UPROPERTY(EditDefaultsOnly, Category = "PR|Dodge")
    float IFrameStartDelay = 0.1f;

    /** 무적 프레임 지속 시간 (기본 0.35초 = 0.45 - 0.1) */
    UPROPERTY(EditDefaultsOnly, Category = "PR|Dodge")
    float IFrameDuration = 0.35f;

    /** 회피 종료 콜백이 누락됐을 때 어빌리티를 종료할 최대 안전 시간 */
    UPROPERTY(EditDefaultsOnly, Category = "PR|Dodge")
    float DodgeDuration = 3.0f;

private:
    /** 회피 종료 이벤트를 구독 중인 AnimInstance */
    UPROPERTY()
    TObjectPtr<UPRAnimInstance> ActiveAnimInstance;

    /** 활성화된 카메라 모디파이어 (구르기 종료 시 비활성화하기 위함) */
    UPROPERTY()
    TObjectPtr<UPRCameraModifier> ActiveCameraModifier;
    
    /** 적용된 무적 효과 핸들 */
    FActiveGameplayEffectHandle InvulnerableEffectHandle;

    /** 회피 최대 안전 종료 타이머 핸들 */
    FTimerHandle DodgeEndTimerHandle;

    /** 무적 시작 타이머 핸들 */
    FTimerHandle IFrameStartTimerHandle;

    /** 무적 종료 타이머 핸들 */
    FTimerHandle IFrameEndTimerHandle;
    
    /** 서버가 데이터를 받았는지 확인하기 위한 플래그 */
    bool bServerReceivedDirection = false;
    FVector SavedDirection = FVector::ZeroVector;
    bool bSavedHasMovementInput = false;
    bool bDodgeEndRequested = false;

    /** 회피 실행을 구분하기 위한 누적 번호 */
    int32 DodgeExecutionId = 0;

    /** 현재 진행 중인 회피 실행 번호 */
    int32 ActiveDodgeExecutionId = 0;
};

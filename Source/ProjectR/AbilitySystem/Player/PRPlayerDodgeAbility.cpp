// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "ProjectR/AbilitySystem/Player/PRPlayerDodgeAbility.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ProjectR/PRGameplayTags.h"

UPRPlayerDodgeAbility::UPRPlayerDodgeAbility()
{
    // 어빌리티 식별 및 입력 태그 설정
    AbilityTags.AddTag(PRGameplayTags::Ability_Player_Dodge);
    ActivationOwnedTags.AddTag(PRGameplayTags::State_Dodging);
    
    // 구르기 중에는 다른 구르기나 사격 등을 차단
    // BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Weapon_Fire_Primary);
    
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UPRPlayerDodgeAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    ACharacter* Character = CastChecked<ACharacter>(ActorInfo->AvatarActor.Get());
    
    // 1. 구르기 방향 결정
    FVector DodgeDirection = Character->GetLastMovementInputVector();
    if (DodgeDirection.IsNearlyZero())
    {
        DodgeDirection = Character->GetActorForwardVector();
    }
    else
    {
        DodgeDirection.Normalize();
    }

    // 캐릭터가 구를 방향을 바라보게 회전 조정 (필요 시)
    Character->SetActorRotation(DodgeDirection.Rotation());

    // 몽타주 재생 태스크
    UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
        this, TEXT("Dodge"), DodgeMontage, 1.0f, NAME_None, false);
    
    MontageTask->OnCompleted.AddDynamic(this, &UPRPlayerDodgeAbility::OnMontageCompleted);
    MontageTask->OnBlendOut.AddDynamic(this, &UPRPlayerDodgeAbility::OnMontageCompleted);
    MontageTask->OnInterrupted.AddDynamic(this, &UPRPlayerDodgeAbility::OnMontageCompleted);
    MontageTask->OnCancelled.AddDynamic(this, &UPRPlayerDodgeAbility::OnMontageCompleted);
    MontageTask->Activate();

    // 무적 프레임 타이머
    UAbilityTask_WaitDelay* IFrameStartTask = UAbilityTask_WaitDelay::WaitDelay(this, IFrameStartDelay);
    IFrameStartTask->OnFinish.AddDynamic(this, &UPRPlayerDodgeAbility::StartIFrame);
    IFrameStartTask->Activate();
}

void UPRPlayerDodgeAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    EndIFrame();
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UPRPlayerDodgeAbility::OnMontageCompleted()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UPRPlayerDodgeAbility::StartIFrame()
{
    if (!GetCurrentActorInfo() || !GetCurrentActorInfo()->AbilitySystemComponent.IsValid()) return;

    if (InvulnerableEffectClass)
    {
        FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(InvulnerableEffectClass);
        InvulnerableEffectHandle = ApplyGameplayEffectSpecToOwner(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, SpecHandle);
    }

    // 종료 타이머
    UAbilityTask_WaitDelay* IFrameEndTask = UAbilityTask_WaitDelay::WaitDelay(this, IFrameDuration);
    IFrameEndTask->OnFinish.AddDynamic(this, &UPRPlayerDodgeAbility::EndIFrame);
    IFrameEndTask->Activate();
}

void UPRPlayerDodgeAbility::EndIFrame()
{
    if (InvulnerableEffectHandle.IsValid())
    {
        BP_RemoveGameplayEffectFromOwnerWithHandle(InvulnerableEffectHandle);
        InvulnerableEffectHandle.Invalidate();
    }
}
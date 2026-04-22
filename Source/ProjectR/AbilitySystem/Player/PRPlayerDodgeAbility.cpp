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
    InputTag = PRGameplayTags::Input_Ability_Dodge;
    
    // 구르기 중에는 다른 구르기나 사격 등을 차단
    // BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Weapon_Fire_Primary);
    
    // 네트워크 설정
    ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
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
    // 로컬 컨트롤러(클라+리슨서버 호스트)의 경우
    if (IsLocallyControlled())
    {
        // [클라이언트] 현재의 입력방향을 계산
        FVector DodgeDir = Character->GetLastMovementInputVector();
        FVector FinalDir = DodgeDir.IsNearlyZero() ? Character->GetActorForwardVector() : DodgeDir.GetSafeNormal();
        
        // 리슨 서버 호스트의 경우 : 서버 권한도 있다면 RPC 없이 데이터 세팅
        if (HasAuthority(&ActivationInfo))
        {
            bServerReceivedDirection = true;
            SavedDirection = FinalDir;
        }
        else // 순수 클라이언트라면 RPC 전송
        {
            Server_SendDodgeDirection(FinalDir);
        }
        
        // 즉시 실행 (클라이언트 예측 또는 호스트 즉시 실행)
        ExecuteDodge(FinalDir);
    }
    else // 서버 측 : 클라이언트의 데이터를 기다림
    {
        // 서버 측: 클라이언트의 RPC가 이미 도착했는지 확인 (Network Race Condition 방지)
        if (bServerReceivedDirection)
        {
            ExecuteDodge(SavedDirection);
        }
        // 도착 안 했다면 Server_SendDodgeDirection_Implementation 에서 대기 후 실행됨
    }
}

void UPRPlayerDodgeAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    EndIFrame();
    bServerReceivedDirection = false;
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UPRPlayerDodgeAbility::Server_SendDodgeDirection_Implementation(FVector_NetQuantize Direction)
{
    // 서버는 클라이언트가 보낸 데이터가 도착했을때 실행
    bServerReceivedDirection = true;
    SavedDirection = Direction;
    
    // 어빌리티가 이미 활성화 상태라면 실행
    if (IsActive())
    {
        ExecuteDodge(Direction);
    }
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

void UPRPlayerDodgeAbility::ExecuteDodge(FVector Direction)
{
    ACharacter* Character = CastChecked<ACharacter>(GetAvatarActorFromActorInfo());
    if (!IsValid(Character))
    {
        return;
    }
    Character->SetActorRotation(Direction.Rotation());
    
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

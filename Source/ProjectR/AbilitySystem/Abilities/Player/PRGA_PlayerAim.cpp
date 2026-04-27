// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "ProjectR/AbilitySystem/Abilities/Player/PRGA_PlayerAim.h"

#include "ProjectR/PRGameplayTags.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "GameFramework/Character.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/Game/PRCameraManager.h"
#include "ProjectR/Player/Components/PRSpringArmComponent.h"
#include "ProjectR/System/PREventManagerSubsystem.h"
#include "ProjectR/System/PREventTypes.h"
#include "ProjectR/UI/Crosshair/PRCrosshairConfig.h"

UPRGA_PlayerAim::UPRGA_PlayerAim()
{
	// 어빌리티 식별 및 입력 태그 설정
	AbilityTags.AddTag(PRGameplayTags::Ability_Player_Aim);
	InputTag = PRGameplayTags::Input_Ability_Aim;

	// [핵심] 조준 중일 때 스스로에게 State.Aiming 태그를 부여합니다.
	// 이 태그가 켜져 있으면 캐릭터가 이를 감지하고 걷기 속도와 카메라 FOV를 자동으로 변경합니다.
	ActivationOwnedTags.AddTag(PRGameplayTags::State_Aiming);

	// 조준은 클라이언트 예측(LocalPredicted)으로 서버의 허락 없이 딜레이 없이 즉각 발동해야 합니다.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	
	// 26.04.26, Yuchan, 테스트를 위해 PlayerCharacter HandleGameplayTagUpdated 함수에서 bIsAiming변수 설정
	// ActivationOwnedTags.AddTag(PRGameplayTags::State_Aiming);
}

void UPRGA_PlayerAim::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                      const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                      const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	//임시구현
	if (AimMontage)
	{
		// true 옵션: 어빌리티가 종료되면 이 태스크도 자동으로 취소됨
		UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, TEXT("AimMontage"), AimMontage, 1.0f, NAME_None, true);

		MontageTask->Activate();
	}
	//----
	// [추가] 내 조준 세팅값을 카메라 컴포넌트들에게 전달 (로컬 플레이어만 적용)
	if (IsLocallyControlled())
	{
		if (APRPlayerCharacter* Character = Cast<APRPlayerCharacter>(GetAvatarActorFromActorInfo()))
		{
			// 1. SpringArm (물리적 거리/위치) 세팅 덮어쓰기
			if (UPRSpringArmComponent* SpringArm = Character->CameraBoom)
			{
				SpringArm->bIsAimingOverride = true;
				SpringArm->AimTargetArmLength = AimTargetArmLength;
				SpringArm->AimTargetOffset = AimTargetOffset;
				SpringArm->AimSocketOffset = AimSocketOffset;
			}

			// 2. CameraManager (시야각 FOV) 세팅 덮어쓰기
			if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
			{
				if (APRCameraManager* CameraManager = Cast<APRCameraManager>(PC->PlayerCameraManager))
				{
					CameraManager->OverrideAimFOV = AimFOV;
				}
			}
		}

		// 3. 26.04.26, Yuchan,UI 알림: 크로스헤어 Config 적용 -> 에이밍 시작 순으로 발송
		// (Config 가 먼저 들어가야 위젯 표시 직전에 비주얼이 갱신됨)
		if (UWorld* World = GetWorld())
		{
			if (UPREventManagerSubsystem* EventMgr = World->GetSubsystem<UPREventManagerSubsystem>())
			{
				FPRChangeCrosshairEventPayload CrosshairPayload;
				CrosshairPayload.Config = AimCrosshairConfig;
				EventMgr->BroadcastTyped(PRGameplayTags::Event_Player_ChangeCrosshair, CrosshairPayload);

				EventMgr->BroadcastEmpty(PRGameplayTags::Event_Player_Aim_Start);
			}
		}
	}
}

void UPRGA_PlayerAim::InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	// 플레이어가 마우스 우클릭(조준 버튼)을 떼면 즉시 어빌리티를 종료합니다.
	// 종료되는 순간 ActivationOwnedTags에 있던 State.Aiming 태그도 자동으로 회수됩니다.
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	Super::InputReleased(Handle, ActorInfo, ActivationInfo);
}

void UPRGA_PlayerAim::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 임시구현
	if (AimMontage)
	{
		if (ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
		{
			if (UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance())
			{
				AnimInstance->Montage_Stop(0.2f, AimMontage);
			}
		}
	}
	// [추가] 조준이 끝났으므로 카메라 덮어쓰기 해제
	if (IsLocallyControlled())
	{
		if (APRPlayerCharacter* Character = Cast<APRPlayerCharacter>(GetAvatarActorFromActorInfo()))
		{
			if (UPRSpringArmComponent* SpringArm = Character->CameraBoom)
			{
				SpringArm->bIsAimingOverride = false;
			}

			if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
			{
				if (APRCameraManager* CameraManager = Cast<APRCameraManager>(PC->PlayerCameraManager))
				{
					CameraManager->OverrideAimFOV = 0.0f;
				}
			}
		}

		// 26.04.26, Yuchan, UI 알림: 에이밍 종료
		if (UWorld* World = GetWorld())
		{
			if (UPREventManagerSubsystem* EventMgr = World->GetSubsystem<UPREventManagerSubsystem>())
			{
				EventMgr->BroadcastEmpty(PRGameplayTags::Event_Player_Aim_End);
			}
		}
	}
	// -----
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

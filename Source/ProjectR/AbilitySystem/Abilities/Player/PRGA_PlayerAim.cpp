// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#include "ProjectR/AbilitySystem/Abilities/Player/PRGA_PlayerAim.h"

#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/Game/PRCameraManager.h"
#include "ProjectR/Player/Components/PRSpringArmComponent.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/System/PREventManagerSubsystem.h"
#include "ProjectR/System/PREventTypes.h"
#include "ProjectR/UI/Crosshair/PRCrosshairConfig.h"
#include "ProjectR/Weapon/Components/PRWeaponManagerComponent.h"

UPRGA_PlayerAim::UPRGA_PlayerAim()
{
	FGameplayTagContainer DefaultAbilityTags;
	DefaultAbilityTags.AddTag(PRGameplayTags::Ability_Player_Aim);
	SetAssetTags(DefaultAbilityTags);

	InputTag = PRGameplayTags::Input_Ability_Aim;
	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Sprint);

	ActivationOwnedTags.AddTag(PRGameplayTags::State_Aiming);

	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
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

	if (APRPlayerCharacter* AvatarCharacter = Cast<APRPlayerCharacter>(GetAvatarActorFromActorInfo()))
	{
		if (UPRWeaponManagerComponent* WeaponManager = AvatarCharacter->GetWeaponManager())
		{
			if (WeaponManager->GetCurrentWeaponSlot()== EPRWeaponSlotType::None)
			{
				EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
				return;
				// TODO: 위 로직 CanActivate로 이동
			}
			if (WeaponManager->GetArmedState() != EPRArmedState::Armed)
			{
				WeaponManager->SetWeaponArmedState(EPRArmedState::Armed);
			}
		}
	}

	if (IsLocallyControlled())
	{
		ApplyAimCameraMode();

		if (APRPlayerCharacter* Character = Cast<APRPlayerCharacter>(GetAvatarActorFromActorInfo()))
		{
			if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
			{
				if (APRCameraManager* CameraManager = Cast<APRCameraManager>(PC->PlayerCameraManager))
				{
					CameraManager->OverrideAimFOV = AimFOV;
				}
			}
		}

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
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	Super::InputReleased(Handle, ActorInfo, ActivationInfo);
}

void UPRGA_PlayerAim::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (IsLocallyControlled())
	{
		if (APRPlayerCharacter* Character = Cast<APRPlayerCharacter>(GetAvatarActorFromActorInfo()))
		{
			if (UPRSpringArmComponent* SpringArm = Character->CameraBoom)
			{
				if (SpringArm->GetCameraMode() == EPRCameraMode::Aim)
				{
					SpringArm->SetCameraMode(EPRCameraMode::Default);
				}
			}

			if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
			{
				if (APRCameraManager* CameraManager = Cast<APRCameraManager>(PC->PlayerCameraManager))
				{
					CameraManager->OverrideAimFOV = 0.0f;
				}
			}
		}

		if (UWorld* World = GetWorld())
		{
			if (UPREventManagerSubsystem* EventMgr = World->GetSubsystem<UPREventManagerSubsystem>())
			{
				EventMgr->BroadcastEmpty(PRGameplayTags::Event_Player_Aim_End);
			}
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UPRGA_PlayerAim::ApplyAimCameraMode()
{
	if (!IsLocallyControlled())
	{
		return;
	}

	APRPlayerCharacter* Character = Cast<APRPlayerCharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(Character))
	{
		return;
	}

	if (UPRSpringArmComponent* SpringArm = Character->CameraBoom)
	{
		SpringArm->SetCameraModeSettings(
			EPRCameraMode::Aim,
			FPRSpringArmCameraModeSettings(AimTargetArmLength, AimTargetOffset, AimSocketOffset));
		SpringArm->SetCameraMode(EPRCameraMode::Aim);
	}
}

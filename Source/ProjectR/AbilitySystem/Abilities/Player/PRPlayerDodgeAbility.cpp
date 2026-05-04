// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#include "ProjectR/AbilitySystem/Abilities/Player/PRPlayerDodgeAbility.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimMontage.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/Player/PRCameraModifier.h"
#include "ProjectR/Player/Components/PRActionInputRouterComponent.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/Weapon/Components/PRWeaponManagerComponent.h"
#include "TimerManager.h"

/*~ 초기화 ~*/

UPRPlayerDodgeAbility::UPRPlayerDodgeAbility()
{
	// 회피 Ability가 활성화되는 동안 GAS 태그로 다른 플레이어 액션을 취소하고 차단한다.
	AbilityTags.AddTag(PRGameplayTags::Ability_Player_Dodge);
	ActivationOwnedTags.AddTag(PRGameplayTags::State_Dodging);
	ActivationBlockedTags.AddTag(PRGameplayTags::State_Dodging);
	InputTag = PRGameplayTags::Input_Ability_Dodge;

	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Weapon_Fire_Primary);
	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Aim);
	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Crouch);

	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Weapon_Fire_Primary);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Aim);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Crouch);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Dodge);

	// 클라이언트 예측으로 몽타주와 루트모션을 즉시 재생하고, 서버에는 발동 순간의 방향만 한 번 전달한다.
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

/*~ UGameplayAbility Interface ~*/

void UPRPlayerDodgeAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
	if (!IsValid(Character))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (IsLocallyControlled())
	{
		// 발동 순간의 이동 입력만 샘플링해서 전방 구르기와 백스텝을 결정한다.
		const FVector InputDirection = Character->GetLastMovementInputVector();
		const bool bHasMovementInput = !InputDirection.IsNearlyZero();
		const FVector FinalDirection = bHasMovementInput
			? InputDirection.GetSafeNormal()
			: -Character->GetActorForwardVector();

		if (HasAuthority(&ActivationInfo))
		{
			// 리슨 서버 로컬 플레이어는 RPC 없이 같은 값을 서버 실행 경로에 저장한다.
			bServerReceivedDirection = true;
			SavedDirection = FinalDirection;
			bSavedHasMovementInput = bHasMovementInput;
		}
		else
		{
			// 원격 클라이언트는 회피 발동 순간의 방향과 입력 여부만 서버로 한 번 전송한다.
			Server_SendDodgeDirection(FinalDirection, bHasMovementInput);
		}

		ExecuteDodge(FinalDirection, bHasMovementInput);
	}
	else if (bServerReceivedDirection)
	{
		// 서버 Ability가 RPC보다 늦게 활성화된 경우 저장된 요청 값으로 실행한다.
		ExecuteDodge(SavedDirection, bSavedHasMovementInput);
	}
}

void UPRPlayerDodgeAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	bDodgeEndRequested = true;

	if (APRPlayerCharacter* PlayerCharacter = Cast<APRPlayerCharacter>(GetAvatarActorFromActorInfo()))
	{
		if (UPRActionInputRouterComponent* ActionInputRouter = PlayerCharacter->GetActionInputRouter())
		{
			ActionInputRouter->UnregisterInputConsumer(this);
		}
	}

	ClearDodgeTimers();
	EndIFrame();
	StopDodgeMontage(DodgeMontageStopBlendOutTime);

	if (IsValid(ActiveMontageTask))
	{
		ActiveMontageTask->EndTask();
		ActiveMontageTask = nullptr;
	}

	if (IsValid(ActiveCameraModifier))
	{
		// 즉시 제거하지 않고 모디파이어의 AlphaOut 설정에 따라 자연스럽게 복구한다.
		ActiveCameraModifier->DisableModifier(false);
		ActiveCameraModifier = nullptr;
	}

	bServerReceivedDirection = false;
	SavedDirection = FVector::ZeroVector;
	bSavedHasMovementInput = false;
	bCanCancelDodgeByInput = false;
	CurrentDodgeInputCancelBlendOutTime = 0.0f;
	ActiveDodgeExecutionId = 0;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

/*~ 입력 스킵 처리 ~*/

bool UPRPlayerDodgeAbility::HandleRoutedActionInput()
{
	if (!IsActive())
	{
		return false;
	}

	if (!bCanCancelDodgeByInput)
	{
		// 회피 중에는 입력을 행동으로 넘기지 않고 소비한다.
		return true;
	}

	// 노티파이 스테이트가 연 구간에서는 입력 한 번으로 몽타주와 Ability를 함께 조기 종료한다.
	bCanCancelDodgeByInput = false;

	const float BlendOutTime = CurrentDodgeInputCancelBlendOutTime > 0.0f
		? CurrentDodgeInputCancelBlendOutTime
		: DodgeInputCancelBlendOutTime;
	bDodgeEndRequested = true;
	StopDodgeMontage(BlendOutTime);
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	return true;
}

void UPRPlayerDodgeAbility::SetRoutedInputCancelWindow(bool bCanCancel, float BlendOutTime)
{
	if (!IsActive() || !bCanCancel)
	{
		bCanCancelDodgeByInput = false;
		CurrentDodgeInputCancelBlendOutTime = 0.0f;
		return;
	}

	bCanCancelDodgeByInput = true;
	CurrentDodgeInputCancelBlendOutTime = BlendOutTime > 0.0f ? BlendOutTime : DodgeInputCancelBlendOutTime;
}

/*~ 서버 요청 수신 ~*/

void UPRPlayerDodgeAbility::Server_SendDodgeDirection_Implementation(FVector_NetQuantize Direction, bool bHasMovementInput)
{
	// 클라이언트 요청이 서버 실행 경로가 사용할 수 있도록 방향 데이터를 저장한다.
	bServerReceivedDirection = true;
	SavedDirection = Direction;
	bSavedHasMovementInput = bHasMovementInput;

	if (IsActive())
	{
		// 서버 Ability가 이미 활성화되어 대기 중이라면 RPC 수신 시점에 바로 실행한다.
		ExecuteDodge(Direction, bHasMovementInput);
	}
}

/*~ 몽타주 종료 처리 ~*/

void UPRPlayerDodgeAbility::HandleDodgeMontageCompleted()
{
	FinishDodge(false);
}

void UPRPlayerDodgeAbility::HandleDodgeMontageInterrupted()
{
	FinishDodge(true);
}

void UPRPlayerDodgeAbility::HandleDodgeSafetyTimeout(int32 FinishedDodgeExecutionId)
{
	if (FinishedDodgeExecutionId != ActiveDodgeExecutionId)
	{
		// 연속 회피 중 이전 실행의 타이머가 늦게 도착하면 현재 회피를 건드리지 않는다.
		return;
	}

	FinishDodge(false);
}

void UPRPlayerDodgeAbility::FinishDodge(bool bWasCancelled)
{
	if (bDodgeEndRequested || !IsActive())
	{
		return;
	}

	bDodgeEndRequested = true;
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bWasCancelled);
}

/*~ 무적 프레임 ~*/

void UPRPlayerDodgeAbility::HandleIFrameStartTimer(int32 FinishedDodgeExecutionId)
{
	if (FinishedDodgeExecutionId != ActiveDodgeExecutionId || bDodgeEndRequested || !IsActive())
	{
		return;
	}

	StartIFrame();

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	FTimerDelegate IFrameEndDelegate;
	IFrameEndDelegate.BindUObject(this, &UPRPlayerDodgeAbility::HandleIFrameEndTimer, FinishedDodgeExecutionId);
	World->GetTimerManager().SetTimer(IFrameEndTimerHandle, IFrameEndDelegate, IFrameDuration, false);
}

void UPRPlayerDodgeAbility::HandleIFrameEndTimer(int32 FinishedDodgeExecutionId)
{
	if (FinishedDodgeExecutionId != ActiveDodgeExecutionId)
	{
		return;
	}

	EndIFrame();
}

void UPRPlayerDodgeAbility::StartIFrame()
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	if (ActorInfo == nullptr || !ActorInfo->AbilitySystemComponent.IsValid())
	{
		return;
	}

	if (IsValid(InvulnerableEffectClass))
	{
		const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(InvulnerableEffectClass);
		InvulnerableEffectHandle = ApplyGameplayEffectSpecToOwner(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, SpecHandle);
	}
}

void UPRPlayerDodgeAbility::EndIFrame()
{
	if (InvulnerableEffectHandle.IsValid())
	{
		BP_RemoveGameplayEffectFromOwnerWithHandle(InvulnerableEffectHandle);
		InvulnerableEffectHandle.Invalidate();
	}
}

void UPRPlayerDodgeAbility::ClearDodgeTimers()
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	World->GetTimerManager().ClearTimer(DodgeEndTimerHandle);
	World->GetTimerManager().ClearTimer(IFrameStartTimerHandle);
	World->GetTimerManager().ClearTimer(IFrameEndTimerHandle);
}

/*~ 회피 실행 ~*/

void UPRPlayerDodgeAbility::ExecuteDodge(FVector Direction, bool bHasMovementInput)
{
	APRPlayerCharacter* Character = Cast<APRPlayerCharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(Character))
	{
		return;
	}

	const FVector SafeDirection = Direction.IsNearlyZero()
		? (bHasMovementInput ? Character->GetActorForwardVector() : -Character->GetActorForwardVector())
		: Direction.GetSafeNormal();
	const EPRDodgeAnimationType DodgeAnimationType = bHasMovementInput
		? EPRDodgeAnimationType::ForwardRoll
		: EPRDodgeAnimationType::BackStep;
	bDodgeEndRequested = false;
	bCanCancelDodgeByInput = false;
	CurrentDodgeInputCancelBlendOutTime = 0.0f;
	ClearDodgeTimers();
	EndIFrame();

	// 회피마다 실행 번호를 갱신해 이전 회피의 타이머 콜백과 현재 회피를 구분한다.
	++DodgeExecutionId;
	ActiveDodgeExecutionId = DodgeExecutionId;

	if (bHasMovementInput)
	{
		// 방향 입력 회피는 캐릭터를 입력 방향으로 돌린 뒤 전방 구르기 몽타주만 재생한다.
		Character->SetActorRotation(SafeDirection.Rotation());
	}

	if (UPRActionInputRouterComponent* ActionInputRouter = Character->GetActionInputRouter())
	{
		ActionInputRouter->RegisterInputConsumer(this);
	}
	PlayDodgeMontage(SelectDodgeMontage(Character, DodgeAnimationType));

	UWorld* World = GetWorld();
	if (IsValid(World))
	{
		// 몽타주 콜백이 누락되어도 Ability가 남지 않도록 안전 종료 타이머를 둔다.
		FTimerDelegate DodgeEndDelegate;
		DodgeEndDelegate.BindUObject(this, &UPRPlayerDodgeAbility::HandleDodgeSafetyTimeout, ActiveDodgeExecutionId);
		World->GetTimerManager().SetTimer(DodgeEndTimerHandle, DodgeEndDelegate, DodgeDuration, false);

		// 무적 프레임도 실행 번호를 물고 시작하므로 오래된 타이머를 무시할 수 있다.
		FTimerDelegate IFrameStartDelegate;
		IFrameStartDelegate.BindUObject(this, &UPRPlayerDodgeAbility::HandleIFrameStartTimer, ActiveDodgeExecutionId);
		World->GetTimerManager().SetTimer(IFrameStartTimerHandle, IFrameStartDelegate, IFrameStartDelay, false);
	}

	if (IsLocallyControlled())
	{
		// 카메라 피드백은 로컬 소유 플레이어에게만 적용한다.
		APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
		if (IsValid(PlayerController) && IsValid(PlayerController->PlayerCameraManager))
		{
			ActiveCameraModifier = Cast<UPRCameraModifier>(
				PlayerController->PlayerCameraManager->AddNewCameraModifier(UPRCameraModifier::StaticClass())
			);

			if (IsValid(ActiveCameraModifier))
			{
				ActiveCameraModifier->SetActionCameraSettings(110.0f, FVector::ZeroVector);
			}
		}
	}
}

UAnimMontage* UPRPlayerDodgeAbility::SelectDodgeMontage(const APRPlayerCharacter* Character, EPRDodgeAnimationType AnimationType) const
{
	const bool bUseForwardRoll = AnimationType == EPRDodgeAnimationType::ForwardRoll;
	if (!IsValid(Character))
	{
		return bUseForwardRoll ? UnarmedForwardRollMontage.Get() : UnarmedBackStepMontage.Get();
	}

	const UPRWeaponManagerComponent* WeaponManager = Character->GetWeaponManager();
	if (!IsValid(WeaponManager)
		|| WeaponManager->GetArmedState() == EPRArmedState::Unarmed
		|| WeaponManager->GetCurrentWeaponSlot() == EPRWeaponSlotType::None)
	{
		return bUseForwardRoll ? UnarmedForwardRollMontage.Get() : UnarmedBackStepMontage.Get();
	}

	if (WeaponManager->GetCurrentWeaponSlot() == EPRWeaponSlotType::Primary)
	{
		UAnimMontage* SelectedMontage = bUseForwardRoll ? PrimaryForwardRollMontage.Get() : PrimaryBackStepMontage.Get();
		return IsValid(SelectedMontage) ? SelectedMontage : (bUseForwardRoll ? UnarmedForwardRollMontage.Get() : UnarmedBackStepMontage.Get());
	}

	if (WeaponManager->GetCurrentWeaponSlot() == EPRWeaponSlotType::Secondary)
	{
		UAnimMontage* SelectedMontage = bUseForwardRoll ? SecondaryForwardRollMontage.Get() : SecondaryBackStepMontage.Get();
		return IsValid(SelectedMontage) ? SelectedMontage : (bUseForwardRoll ? UnarmedForwardRollMontage.Get() : UnarmedBackStepMontage.Get());
	}

	return bUseForwardRoll ? UnarmedForwardRollMontage.Get() : UnarmedBackStepMontage.Get();
}

void UPRPlayerDodgeAbility::PlayDodgeMontage(UAnimMontage* DodgeMontage)
{
	if (!IsValid(DodgeMontage))
	{
		FinishDodge(true);
		return;
	}

	ActiveDodgeMontage = DodgeMontage;
	ActiveMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		DodgeMontage,
		FMath::Max(DodgeMontagePlayRate, UE_SMALL_NUMBER));

	if (!IsValid(ActiveMontageTask))
	{
		FinishDodge(true);
		return;
	}

	// 몽타주 종료 콜백은 회피 Ability 종료의 주 경로다.
	ActiveMontageTask->OnCompleted.AddDynamic(this, &UPRPlayerDodgeAbility::HandleDodgeMontageCompleted);
	ActiveMontageTask->OnBlendOut.AddDynamic(this, &UPRPlayerDodgeAbility::HandleDodgeMontageCompleted);
	ActiveMontageTask->OnInterrupted.AddDynamic(this, &UPRPlayerDodgeAbility::HandleDodgeMontageInterrupted);
	ActiveMontageTask->OnCancelled.AddDynamic(this, &UPRPlayerDodgeAbility::HandleDodgeMontageInterrupted);
	ActiveMontageTask->ReadyForActivation();
}

void UPRPlayerDodgeAbility::StopDodgeMontage(float BlendOutTime)
{
	if (!IsValid(ActiveDodgeMontage))
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->CurrentMontageStop(BlendOutTime);
	}

	ActiveDodgeMontage = nullptr;
}

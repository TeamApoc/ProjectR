// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#include "ProjectR/AbilitySystem/Abilities/Player/PRPlayerDodgeAbility.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "ProjectR/Animation/PRAnimInstance.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/Player/PRCameraModifier.h"
#include "ProjectR/PRGameplayTags.h"
#include "TimerManager.h"

/*~ 초기화 ~*/

UPRPlayerDodgeAbility::UPRPlayerDodgeAbility()
{
	// 어빌리티 식별 및 입력 태그 설정
	AbilityTags.AddTag(PRGameplayTags::Ability_Player_Dodge);
	ActivationOwnedTags.AddTag(PRGameplayTags::State_Dodging);
	InputTag = PRGameplayTags::Input_Ability_Dodge;

	// 구르기 중에는 다른 구르기나 사격 등을 차단
	// BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Weapon_Fire_Primary);

	// 클라이언트 예측과 서버 권위 실행을 함께 사용한다
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

/*~ UGameplayAbility Interface ~*/

void UPRPlayerDodgeAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
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
		// 회피 입력 순간에만 방향을 샘플링한다.
		// 이후 서버에는 이 값만 한 번 보내고, 로컬은 같은 값으로 즉시 예측 실행한다.
		// 입력이 있으면 입력 방향 전방 구르기, 입력이 없으면 현재 방향 기준 백스텝으로 구분한다.
		const FVector InputDirection = Character->GetLastMovementInputVector();
		const bool bHasMovementInput = !InputDirection.IsNearlyZero();
		const FVector FinalDirection = bHasMovementInput
			? InputDirection.GetSafeNormal()
			: -Character->GetActorForwardVector();

		if (HasAuthority(&ActivationInfo))
		{
			// 리슨 서버 플레이어는 RPC 없이 서버 실행에 필요한 요청 값을 직접 저장한다.
			bServerReceivedDirection = true;
			SavedDirection = FinalDirection;
			bSavedHasMovementInput = bHasMovementInput;
		}
		else
		{
			// 순수 클라이언트는 회피 순간의 방향과 입력 유무만 서버에 1회 전달한다.
			Server_SendDodgeDirection(FinalDirection, bHasMovementInput);
		}

		// 로컬 예측으로 애니메이션 요청과 무적 타이머를 즉시 시작한다.
		ExecuteDodge(FinalDirection, bHasMovementInput);
	}
	else
	{
		if (bServerReceivedDirection)
		{
			// 서버 Ability가 RPC보다 늦게 활성화된 경우 저장된 요청 값으로 실행한다.
			ExecuteDodge(SavedDirection, bSavedHasMovementInput);
		}
	}
}

void UPRPlayerDodgeAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	ClearDodgeTimers();
	EndIFrame();
	bServerReceivedDirection = false;
	SavedDirection = FVector::ZeroVector;
	bSavedHasMovementInput = false;
	ActiveDodgeExecutionId = 0;

	if (IsValid(ActiveAnimInstance))
	{
		ActiveAnimInstance->OnDodgeAnimationFinished.RemoveDynamic(this, &UPRPlayerDodgeAbility::HandleDodgeAnimationFinished);
		ActiveAnimInstance->CancelDodge();
		ActiveAnimInstance = nullptr;
	}

	if (IsValid(ActiveCameraModifier))
	{
		// 즉시 제거하지 않고 모디파이어의 AlphaOut 설정에 맞춰 자연스럽게 복구한다
		ActiveCameraModifier->DisableModifier(false);
		ActiveCameraModifier = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

/*~ 서버 요청 수신 ~*/

void UPRPlayerDodgeAbility::Server_SendDodgeDirection_Implementation(FVector_NetQuantize Direction, bool bHasMovementInput)
{
	// 클라이언트 요청을 서버 실행 경로가 사용할 수 있도록 저장한다.
	bServerReceivedDirection = true;
	SavedDirection = Direction;
	bSavedHasMovementInput = bHasMovementInput;

	if (IsActive())
	{
		// 서버 Ability가 이미 활성화되어 대기 중이었다면, RPC 수신 시점에 바로 서버 실행을 이어간다.
		ExecuteDodge(Direction, bHasMovementInput);
	}
}

/*~ 회피 종료 처리 ~*/

void UPRPlayerDodgeAbility::HandleDodgeAnimationFinished()
{
	if (bDodgeEndRequested || !IsActive())
	{
		return;
	}

	bDodgeEndRequested = true;
	const bool bWasCancelledByInput = IsValid(ActiveAnimInstance) && ActiveAnimInstance->WasDodgeInputCancelRequested();
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bWasCancelledByInput);
}

void UPRPlayerDodgeAbility::HandleDodgeSafetyTimeout(int32 FinishedDodgeExecutionId)
{
	if (FinishedDodgeExecutionId != ActiveDodgeExecutionId)
	{
		// 연속 회피 중 이전 실행의 안전 타이머가 늦게 끝난 경우 현재 회피를 건드리지 않는다.
		return;
	}

	HandleDodgeAnimationFinished();
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
	ClearDodgeTimers();
	EndIFrame();

	// 새 회피마다 실행 번호를 갱신해서 이전 회피의 타이머 콜백과 현재 회피를 구분한다.
	++DodgeExecutionId;
	ActiveDodgeExecutionId = DodgeExecutionId;

	if (bHasMovementInput)
	{
		// 방향 입력 회피는 캐릭터를 입력 방향으로 돌린 뒤 전방 구르기 애니메이션만 재생한다.
		Character->SetActorRotation(SafeDirection.Rotation());
	}

	// 애니메이션 종료 Notify가 Ability 종료를 호출할 수 있도록 이벤트를 연결한다.
	BindDodgeAnimationFinished(Character);

	// 실제 회피 몽타주 선택과 재생은 메인 AnimInstance가 담당한다.
	Character->RequestDodgeAnimation(SafeDirection, DodgeAnimationType);

	UWorld* World = GetWorld();
	if (IsValid(World))
	{
		// 몽타주 종료 델리게이트가 누락되어도 Ability가 남지 않도록 최대 지속 시간을 둔다.
		FTimerDelegate DodgeEndDelegate;
		DodgeEndDelegate.BindUObject(this, &UPRPlayerDodgeAbility::HandleDodgeSafetyTimeout, ActiveDodgeExecutionId);
		World->GetTimerManager().SetTimer(DodgeEndTimerHandle, DodgeEndDelegate, DodgeDuration, false);

		// 무적 프레임도 실행 번호를 물고 시작하므로 연속 회피 중 오래된 콜백은 무시된다.
		FTimerDelegate IFrameStartDelegate;
		IFrameStartDelegate.BindUObject(this, &UPRPlayerDodgeAbility::HandleIFrameStartTimer, ActiveDodgeExecutionId);
		World->GetTimerManager().SetTimer(IFrameStartTimerHandle, IFrameStartDelegate, IFrameStartDelay, false);
	}

	if (IsLocallyControlled())
	{
		// 카메라 피드백은 로컬 소유 플레이어에게만 적용한다.
		APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
		if (IsValid(PlayerController))
		{
			APlayerCameraManager* CameraManager = PlayerController->PlayerCameraManager;
			if (IsValid(CameraManager))
			{
				ActiveCameraModifier = Cast<UPRCameraModifier>(
					CameraManager->AddNewCameraModifier(UPRCameraModifier::StaticClass())
				);

				if (IsValid(ActiveCameraModifier))
				{
					// 구르기 중 시야 전환을 강조하기 위한 임시 카메라 수치다.
					ActiveCameraModifier->SetActionCameraSettings(110.0f, FVector(0.0f, 0.0f, 0.0f));
				}
			}
		}
	}
}

/*~ 애니메이션 이벤트 연결 ~*/

void UPRPlayerDodgeAbility::BindDodgeAnimationFinished(ACharacter* Character)
{
	if (!IsValid(Character))
	{
		return;
	}

	USkeletalMeshComponent* MeshComponent = Character->GetMesh();
	if (!IsValid(MeshComponent))
	{
		return;
	}

	ActiveAnimInstance = Cast<UPRAnimInstance>(MeshComponent->GetAnimInstance());
	if (!IsValid(ActiveAnimInstance))
	{
		return;
	}

	// 같은 Ability 인스턴스가 중복 바인딩되지 않도록 기존 바인딩을 지운 뒤 다시 연결한다.
	ActiveAnimInstance->OnDodgeAnimationFinished.RemoveDynamic(this, &UPRPlayerDodgeAbility::HandleDodgeAnimationFinished);
	ActiveAnimInstance->OnDodgeAnimationFinished.AddDynamic(this, &UPRPlayerDodgeAbility::HandleDodgeAnimationFinished);
}

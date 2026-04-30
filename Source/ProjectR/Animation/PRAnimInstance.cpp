// Fill out your copyright notice in the Description page of Project Settings.


#include "PRAnimInstance.h"

#include "Animation/AnimMontage.h"
#include "KismetAnimationLibrary.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/Weapon/Components/PRWeaponManagerComponent.h"

/*~ 초기화 및 업데이트 ~*/

void UPRAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	
	PlayerCharacter = Cast<APRPlayerCharacter>(GetOwningActor());
	if (IsValid(PlayerCharacter))
	{
		CharacterMovement = PlayerCharacter->GetCharacterMovement();
	}
}

void UPRAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	
	if (!IsValid(PlayerCharacter) || !IsValid(CharacterMovement))
	{
		return;
	}
	
	UpdateVelocity();
	UpdateAcceleration();
	UpdateDirection();
	UpdateFlags();
	UpdateMovementMode();
	UpdateTurnInPlace();
	UpdateRootYawOffset();
	DetermineTargetTurnAngle();
	UpdateAim();
	UpdateDodge(DeltaSeconds);
}

void UPRAnimInstance::NativePostEvaluateAnimation()
{
	Super::NativePostEvaluateAnimation();

	// AnimGraph가 한 번 평가한 뒤 일회성 회피 요청 플래그를 내린다.
	ClearDodgeRequestFlags();
}

/*~ 회피 요청 ~*/

void UPRAnimInstance::RequestDodge(const FVector& WorldDirection, EPRDodgeAnimationType AnimationType)
{
	// Character와 Ability는 회피를 언제, 어느 방향으로 할지만 결정한다.
	// 실제 방향 캐싱, 몽타주 선택, 루트모션 재생은 메인 AnimInstance에서 한 번에 처리한다.
	if (!IsValid(PlayerCharacter))
	{
		PlayerCharacter = Cast<APRPlayerCharacter>(GetOwningActor());
	}

	if (!IsValid(PlayerCharacter))
	{
		return;
	}

	const FVector SafeWorldDirection = WorldDirection.IsNearlyZero()
		? PlayerCharacter->GetActorForwardVector()
		: WorldDirection.GetSafeNormal();
	
	// 현재 프로젝트의 실제 회피 모션은 전방 구르기와 백스텝 두 종류만 사용한다.
	DodgeAnimationType = AnimationType;
	bIsDodgeForwardRoll = DodgeAnimationType == EPRDodgeAnimationType::ForwardRoll;
	bIsDodgeBackStep = DodgeAnimationType == EPRDodgeAnimationType::BackStep;

	if (const UPRWeaponManagerComponent* WeaponManager = PlayerCharacter->GetWeaponManager())
	{
		// 회피 순간의 무장 상태를 기준으로 맞는 몽타주를 고르기 위해 최신 무기 상태를 다시 읽는다.
		ArmedState = WeaponManager->GetArmedState();
		EquippedWeaponSlot = WeaponManager->GetCurrentWeaponSlot();
		AimOffsetWeaponSlot = WeaponManager->GetAimOffsetWeaponSlot();
	}

	// 같은 상태 안에서 연속 요청을 감지할 수 있도록 요청 번호와 펄스를 함께 갱신한다.
	DodgeElapsedTime = 0.0f;
	CurrentDodgeAutoFinishTime = FMath::Max(DodgeAutoFinishTime, 1.0f);
	bIsDodging = true;
	bDodgeRequested = true;
	++DodgeRequestId;

	// 루트모션이 CharacterMovement까지 전달되도록 메인 AnimInstance에서 회피 몽타주를 직접 재생한다.
	PlayDodgeMontage();
}

void UPRAnimInstance::FinishDodge()
{
	if (!bIsDodging)
	{
		return;
	}

	bIsDodging = false;
	bDodgeRequested = false;
	bIsDodgeForwardRoll = false;
	bIsDodgeBackStep = false;
	DodgeElapsedTime = 0.0f;
	CurrentDodgeAutoFinishTime = 0.0f;

	// Ability가 회피 상태와 무적 타이머를 정리할 수 있도록 종료 이벤트를 발행한다.
	OnDodgeAnimationFinished.Broadcast();
}

void UPRAnimInstance::CancelDodge()
{
	StopDodgeMontage(DodgeMontageStopBlendOutTime);

	bIsDodging = false;
	bDodgeRequested = false;
	bIsDodgeForwardRoll = false;
	bIsDodgeBackStep = false;
	DodgeElapsedTime = 0.0f;
	CurrentDodgeAutoFinishTime = 0.0f;
}

/*~ 회피 몽타주 재생 ~*/

void UPRAnimInstance::PlayDodgeMontage()
{
	// 현재 무장 슬롯과 회피 타입에 맞는 메인 AnimInstance 몽타주를 선택한다.
	UAnimMontage* DodgeMontage = GetDodgeMontage();
	if (!IsValid(DodgeMontage))
	{
		return;
	}

	if (IsValid(ActiveDodgeMontage) && Montage_IsPlaying(ActiveDodgeMontage.Get()))
	{
		// 새 회피 요청이 이전 회피 몽타주 위에 들어오면 짧게 정리한 뒤 새 몽타주를 시작한다.
		// StopDodgeMontage가 현재 몽타주 정보를 먼저 비워 두므로, 이전 몽타주의 Interrupted 콜백이 새 회피를 끝내지 못한다.
		StopDodgeMontage(0.05f);
	}

	// 메인 AnimInstance에서 재생해야 Root Motion From Montages Only 설정에서 캡슐 이동까지 반영된다.
	const float PlayedDuration = Montage_Play(
		DodgeMontage,
		DodgeMontagePlayRate,
		EMontagePlayReturnType::Duration,
		0.0f,
		true
	);
	if (PlayedDuration <= 0.0f)
	{
		ActiveDodgeMontage = nullptr;
		ActiveDodgeMontageRequestId = 0;
		return;
	}

	ActiveDodgeMontage = DodgeMontage;
	ActiveDodgeMontageRequestId = DodgeRequestId;
	CurrentDodgeAutoFinishTime = FMath::Max(DodgeAutoFinishTime, PlayedDuration + DodgeMontageAutoFinishPadding);

	// 노티파이가 없어도 몽타주 종료 시 Ability까지 회피 종료가 전달되도록 종료 델리게이트를 연결한다.
	FOnMontageEnded MontageEndedDelegate;
	MontageEndedDelegate.BindUObject(this, &UPRAnimInstance::HandleDodgeMontageEnded, DodgeRequestId);
	Montage_SetEndDelegate(MontageEndedDelegate, DodgeMontage);
}

void UPRAnimInstance::StopDodgeMontage(float BlendOutTime)
{
	if (!IsValid(ActiveDodgeMontage))
	{
		return;
	}

	UAnimMontage* MontageToStop = ActiveDodgeMontage.Get();
	ActiveDodgeMontage = nullptr;
	ActiveDodgeMontageRequestId = 0;

	// 회피 흐름에서 시작한 몽타주만 멈춰서 다른 Slot 몽타주에 영향이 가지 않도록 한다.
	Montage_Stop(BlendOutTime, MontageToStop);
}

UAnimMontage* UPRAnimInstance::GetDodgeMontage() const
{
	const bool bUseForwardRoll = bIsDodgeForwardRoll;
	UAnimMontage* SelectedMontage = nullptr;

	if (ArmedState == EPRArmedState::Unarmed || EquippedWeaponSlot == EPRWeaponSlotType::None)
	{
		return bUseForwardRoll ? UnarmedForwardRollMontage.Get() : UnarmedBackStepMontage.Get();
	}

	if (EquippedWeaponSlot == EPRWeaponSlotType::Primary)
	{
		SelectedMontage = bUseForwardRoll ? PrimaryForwardRollMontage.Get() : PrimaryBackStepMontage.Get();
		if (IsValid(SelectedMontage))
		{
			return SelectedMontage;
		}
	}

	if (EquippedWeaponSlot == EPRWeaponSlotType::Secondary)
	{
		SelectedMontage = bUseForwardRoll ? SecondaryForwardRollMontage.Get() : SecondaryBackStepMontage.Get();
		if (IsValid(SelectedMontage))
		{
			return SelectedMontage;
		}
	}
	// 슬롯별 몽타주가 비어 있으면 비무장 몽타주를 fallback으로 사용해 테스트와 임시 세팅을 돕는다.
	return bUseForwardRoll ? UnarmedForwardRollMontage.Get() : UnarmedBackStepMontage.Get();
}

void UPRAnimInstance::HandleDodgeMontageEnded(UAnimMontage* Montage, bool bInterrupted, int32 EndedDodgeRequestId)
{
	if (bInterrupted)
	{
		// 연속 회피로 이전 몽타주가 끊긴 상황은 정상 흐름이므로 종료 이벤트로 취급하지 않는다.
		return;
	}

	// 이전 회피 요청의 블렌드아웃 콜백이 늦게 들어오면 새 회피 몽타주를 끊지 않도록 무시한다.
	if (EndedDodgeRequestId != ActiveDodgeMontageRequestId)
	{
		return;
	}

	// 다른 몽타주의 종료 콜백이 들어온 경우 현재 회피 흐름과 무관하므로 무시한다.
	if (Montage != ActiveDodgeMontage.Get())
	{
		return;
	}

	ActiveDodgeMontage = nullptr;
	ActiveDodgeMontageRequestId = 0;

	if (!bIsDodging)
	{
		return;
	}

	// 몽타주가 자연 종료되거나 외부에서 끊긴 경우에도 Ability가 회피 상태를 정리할 수 있도록 전달한다.
	FinishDodge();
}

/*~ 이동 상태 갱신 ~*/

void UPRAnimInstance::UpdateVelocity()
{
	Velocity = CharacterMovement->Velocity;
	Velocity2D = FVector(Velocity.X, Velocity.Y, 0.0f);
	XYSpeed = Velocity.Size2D();
	
	FRotator ActorRotation = PlayerCharacter->GetActorRotation();
	LocalVelocity2D = ActorRotation.UnrotateVector(Velocity2D);
}

void UPRAnimInstance::UpdateAcceleration()
{
	Acceleration = CharacterMovement->GetCurrentAcceleration();
	
	FRotator ActorRotation = PlayerCharacter->GetActorRotation();
	FVector AccelerationXY = FVector(Acceleration.X, Acceleration.Y, 0.0f); 
	LocalAcceleration2D = ActorRotation.UnrotateVector(AccelerationXY);
}

void UPRAnimInstance::UpdateDirection()
{
	if (XYSpeed < MoveSpeedThreshold) return;

	FRotator ActorRotation = PlayerCharacter->GetActorRotation();
	// 실제 이동 방향 (-180 ~ 180)
	Direction = UKismetAnimationLibrary::CalculateDirection(Velocity2D, ActorRotation);

	// BS를 사용하더라도 Orientation Warping을 섞어주면 발 미끄러짐이 완전히 사라집니다.
	// 6방향 중 가장 가까운 기준각과의 차이(Delta)를 계산합니다.
	float TargetRefAngle = 0.0f;
	if (FMath::Abs(Direction) <= 22.5f) TargetRefAngle = 0.0f;           // F
	else if (Direction > 22.5f && Direction <= 67.5f) TargetRefAngle = 45.0f;   // FR
	else if (Direction > 67.5f && Direction <= 112.5f) TargetRefAngle = 90.0f;  // R (BS가 섞어줌)
	else if (Direction > 112.5f && Direction <= 157.5f) TargetRefAngle = 135.0f; // BR
	else if (FMath::Abs(Direction) > 157.5f) TargetRefAngle = 180.0f;           // B
	else if (Direction < -22.5f && Direction >= -67.5f) TargetRefAngle = -45.0f; // FL
	else if (Direction < -67.5f && Direction >= -112.5f) TargetRefAngle = -90.0f; // L (BS가 섞어줌)
	else if (Direction < -112.5f && Direction >= -157.5f) TargetRefAngle = -135.0f; // BL

	// 이 값을 Warping 노드에 넣으면 BS의 애니메이션 오차를 보정합니다.
	F_OrientationAngle = FRotator::NormalizeAxis(Direction - TargetRefAngle);
}

void UPRAnimInstance::UpdateFlags()
{
	bHasAcceleration = !LocalAcceleration2D.IsNearlyZero();
	bShouldMove = (XYSpeed > MoveSpeedThreshold) && bHasAcceleration;
	bIsFalling = CharacterMovement->IsFalling();
	bIsCrouching = PlayerCharacter->IsCrouching();
	bIsSprint = PlayerCharacter->IsSprinting();
	bIsAiming = PlayerCharacter->IsAiming();
	bIsWalking = PlayerCharacter->IsWalking();
	bIsStrafeMode = bIsAiming || bIsWalking;
	// LandState = ELandState::Normal; // TODO: Character 상태 적용
	
	// Falling 에서 Ground로 전환되는 순간 감지
	if (bIsFalling)
	{
		LandState = ELandState::None;
	}
	else if (LandState == ELandState::None)
	{
		LandState = ELandState::Normal;
	}
}

void UPRAnimInstance::UpdateMovementMode()
{
	if (!bShouldMove)
	{
		MovementMode = EPRMovementMode::Idle;
		return;
	}
	
	if (bIsSprint)
	{
		MovementMode = EPRMovementMode::Sprinting;
		return;
	}
	
	// 캐릭터의 실제 값을 가져와 movementmode 판정 (walk 속도와 jog속도의 절반이 넘어가면 애니메이션 교체)
	const float Threshold = (PlayerCharacter->GetWalkSpeed() + PlayerCharacter->GetJogSpeed()) * 0.5f;
	MovementMode = (XYSpeed <= Threshold) ? EPRMovementMode::Walking : EPRMovementMode::Jogging;
}


/*~ 회전 및 조준 상태 갱신 ~*/

void UPRAnimInstance::UpdateTurnInPlace()
{
    // 커브 값 추출
	const float TurnYawWeight = GetCurveValue(TurnYawWeightCurveName);
	const bool bWasTurning = bIsTurningInPlace;

	// 커브 값이 존재하면 턴 중으로 판단
	bIsTurningInPlace = TurnYawWeight > 0.1f;
	
	// 턴이 시작되는 순간의 오프셋을 스냅샷으로 저장
	if (!bWasTurning && bIsTurningInPlace)
	{
		TurnStartYaw = RootYawOffset;
	}

	// 만약 회전이 끝났다면 TargetTurnAngle을 None으로 초기화하여 다음 회전을 준비합니다.
	if (bWasTurning && !bIsTurningInPlace)
	{
		TargetTurnAngle = EPRTurnAngle::None;
	}
}


void UPRAnimInstance::UpdateRootYawOffset()
{
    float DeltaSeconds = GetWorld()->GetDeltaSeconds();
    
    // 1. 카메라와 캐릭터의 실제 각도 차이 계산
    FRotator ActorRot = PlayerCharacter->GetActorRotation();
    FRotator AimRot = PlayerCharacter->GetBaseAimRotation();
    float CurrentDiff = UKismetMathLibrary::NormalizedDeltaRotator(AimRot, ActorRot).Yaw;

    if (bShouldMove || bIsFalling)
    {
        RootYawOffset = FMath::FInterpTo(RootYawOffset, 0.0f, DeltaSeconds, RootYawOffsetInterpSpeed);
        return;
    }

    // 2. 턴 상태 및 커브 확인
    const float TurnSwitch = GetCurveValue(TurnYawWeightCurveName); // 1 or 0
    const float RotationAlpha = GetCurveValue(TurnRotationCurveName); // 0.0 -> 1.0 (새로 만드실 커브)
    
    const bool bWasTurning = bIsTurningInPlace;
    bIsTurningInPlace = TurnSwitch > 0.5f;

    if (bIsTurningInPlace)
    {
        // [턴 중] 몽타주가 캡슐을 돌리는 동안, 메시는 반대 방향으로 버티게(Counter-Rotate) 합니다.
        if (!bWasTurning)
        {
            // 턴 시작 시점의 실제 각도 차이를 저장 (예: 90도)
            TurnStartYaw = CurrentDiff;
        }

        // 커브(Alpha)가 0에서 1로 갈수록 오프셋을 90 -> 0으로 줄임
        // 이 로직이 있어야 캡슐이 도는 동안 메시가 발 미끄러짐 없이 제자리에서 도는 것처럼 보입니다.
        RootYawOffset = TurnStartYaw * (1.0f - FMath::Clamp(RotationAlpha, 0.0f, 1.0f));
    }
    else
    {
        // [턴 대기 중 또는 Relax 상태]
        // 캡슐과 메시를 일치시킵니다 (오프셋 0). 이렇게 하면 몸이 카메라를 따라가지 않습니다.
        RootYawOffset = 0.0f;
    }
    
    RemainingTurnYaw = RootYawOffset;
}

void UPRAnimInstance::UpdateAim()
{
    // 실제 카메라 차이에서 몸이 돌아간 만큼을 뺍니다.
    // 이렇게 해야 Strafe 모드에서 값이 두 배가 되지 않고 정확한 목표를 바라봅니다.
    FRotator ActorRot = PlayerCharacter->GetActorRotation();
    FRotator AimRot = PlayerCharacter->GetBaseAimRotation();
    float CurrentDiff = UKismetMathLibrary::NormalizedDeltaRotator(AimRot, ActorRot).Yaw;

	ArmedState = EPRArmedState::Unarmed;
	EquippedWeaponSlot = EPRWeaponSlotType::None;
	AimOffsetWeaponSlot = EPRWeaponSlotType::None;

	if (IsValid(PlayerCharacter))
	{
		if (const UPRWeaponManagerComponent* WeaponManager = PlayerCharacter->GetWeaponManager())
		{
			ArmedState = WeaponManager->GetArmedState();
			EquippedWeaponSlot = WeaponManager->GetCurrentWeaponSlot();
			AimOffsetWeaponSlot = WeaponManager->GetAimOffsetWeaponSlot();
		}
	}
	
    if (bIsStrafeMode)
    {
        // 턴 중에는 몸이 돌아가는 만큼 고개 각도를 보정하여 목표 고정
        AimYaw = CurrentDiff - RootYawOffset;
    }
    else
    {
        // Relax: 90도 전까지만 고개를 돌리고, 넘어가면 0으로 복구 (고개 정면)
        if (FMath::Abs(CurrentDiff) < TurnThreshold - 5.0f)
        {
            AimYaw = CurrentDiff;
        }
        else
        {
            AimYaw = FMath::FInterpTo(AimYaw, 0.0f, GetWorld()->GetDeltaSeconds(), 5.0f);
        }
    }

    AimPitch = UKismetMathLibrary::NormalizedDeltaRotator(AimRot, ActorRot).Pitch;
}



void UPRAnimInstance::DetermineTargetTurnAngle()
{
	if (bIsTurningInPlace || !bIsStrafeMode)
	{
		TargetTurnAngle = EPRTurnAngle::None;
		return;
	}

	// [중요] 이제 RootYawOffset이 아닌 실제 각도 차이(CurrentDiff)를 기준으로 판정합니다.
	FRotator ActorRot = PlayerCharacter->GetActorRotation();
	FRotator AimRot = PlayerCharacter->GetBaseAimRotation();
	float AbsDiff = FMath::Abs(UKismetMathLibrary::NormalizedDeltaRotator(AimRot, ActorRot).Yaw);

	if (AbsDiff > TurnThreshold)
	{
		TargetTurnAngle = EPRTurnAngle::Angle90;
		// 이후 ABP 스테이트 머신에서 TargetTurnAngle == Angle90 조건을 통해 턴 시작
	}
	else
	{
		TargetTurnAngle = EPRTurnAngle::None;
	}
}

/*~ 회피 상태 갱신 ~*/

void UPRAnimInstance::UpdateDodge(float DeltaSeconds)
{
	if (!bIsDodging)
	{
		return;
	}

	// 기본 종료 경로는 몽타주 종료 델리게이트다.
	// 이 타이머는 몽타주 설정 오류나 델리게이트 누락 시 상태가 남는 것을 막는 보조 안전장치다.
	DodgeElapsedTime += DeltaSeconds;
	const float AutoFinishTime = CurrentDodgeAutoFinishTime > 0.0f
		? CurrentDodgeAutoFinishTime
		: DodgeAutoFinishTime;

	if (DodgeElapsedTime >= AutoFinishTime)
	{
		// Anim Notify가 누락되어도 AnimInstance와 Ability 상태가 남지 않도록 자동 종료한다.
		FinishDodge();
	}
}

void UPRAnimInstance::ClearDodgeRequestFlags()
{
	bDodgeRequested = false;
}

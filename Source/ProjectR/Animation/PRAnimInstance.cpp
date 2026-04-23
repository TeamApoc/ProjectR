// Fill out your copyright notice in the Description page of Project Settings.


#include "PRAnimInstance.h"

#include "KismetAnimationLibrary.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "ProjectR/Character/PRPlayerCharacter.h"

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
}

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
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
	UpdateLean();
	UpdateFlags();
	UpdateMovementMode();
	UpdateTurnInPlace();
	UpdateRootYawOffset();
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
	FRotator ActorRotation = PlayerCharacter->GetActorRotation();
	
	if (!bIsAiming)
	{
		Direction = 0.0f;
		CardinalDirection = ECardinalDirection::Forward;
		
		F_OrientationAngle = 0.0f;
		R_OrientationAngle = 0.0f;
		B_OrientationAngle = 0.0f;
		L_OrientationAngle = 0.0f;
		return;
	}
	
	Direction =  UKismetAnimationLibrary::CalculateDirection(Velocity2D,ActorRotation);
	if (Direction >= -70.f && Direction <= 70.f)
	{
		CardinalDirection = ECardinalDirection::Forward;
	}
	else if (Direction > 70.f && Direction <= 110.f)
	{
		CardinalDirection = ECardinalDirection::Right;
	}
	else if (Direction >= -110.f && Direction < -70.f)
	{
		CardinalDirection = ECardinalDirection::Left;
	}
	else
	{
		CardinalDirection = ECardinalDirection::Backward;
	}
	
	F_OrientationAngle = Direction;
	R_OrientationAngle = Direction - 90.f;
	B_OrientationAngle = Direction - 180.f;
	L_OrientationAngle = Direction + 90.f;
}

void UPRAnimInstance::UpdateFlags()
{
	bHasAcceleration = !LocalAcceleration2D.IsNearlyZero();
	bShouldMove = (XYSpeed > MoveSpeedThreshold) && bHasAcceleration;
	bIsFalling = CharacterMovement->IsFalling();
	bIsCrouching = PlayerCharacter->IsCrouching();
	bIsSprint = PlayerCharacter->IsSprinting();
	bIsAiming = PlayerCharacter->IsAiming();
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

void UPRAnimInstance::UpdateAim()
{
	FRotator AimRotation = PlayerCharacter->GetBaseAimRotation();
	FRotator ActorRotation = PlayerCharacter->GetActorRotation();
	FRotator DeltaRotator = UKismetMathLibrary::NormalizedDeltaRotator(AimRotation, ActorRotation);
    
	AimPitch = DeltaRotator.Pitch;
	AimYaw = -RootYawOffset;
}

void UPRAnimInstance::UpdateLean()
{
	float LookDirection = PlayerCharacter->GetDesiredLookDirection();
	float DeltaSeconds = GetWorld()->GetDeltaSeconds();
	LeanDirection = FMath::FInterpTo(LeanDirection, LookDirection, DeltaSeconds, 10.f);
}

void UPRAnimInstance::UpdateTurnInPlace()
{
    // 커브 값 추출
    const float TurnYawWeight = GetCurveValue(TurnYawWeightCurveName);
    const float CurveRemainingTurnYaw = GetCurveValue(RemainingTurnYawCurveName);
    
	// 조준 중이 아닐 때만 제자리 회전 애니메이션이 활성화되도록 제어
	bIsTurningInPlace = TurnYawWeight > 0.0f;
    
    // Distance 커브 업데이트 (RemainingTurnYaw 사용)
    LastDistanceCurve = DistanceCurve;
    DistanceCurve = CurveRemainingTurnYaw;
}

void UPRAnimInstance::UpdateRootYawOffset()
{
	float DeltaSeconds = GetWorld()->GetDeltaSeconds();
    
	LastMovingRotation = MovingRotation;
	MovingRotation = PlayerCharacter->GetActorRotation();
    
	if (bShouldMove || bIsFalling || bIsAiming) 
	{
		RootYawOffset = FMath::FInterpTo(RootYawOffset, 0.0f, DeltaSeconds, RootYawOffsetInterpSpeed);
	}
	else
	{
		FRotator DeltaRotator = UKismetMathLibrary::NormalizedDeltaRotator(MovingRotation, LastMovingRotation);
		RootYawOffset -= DeltaRotator.Yaw;
		RootYawOffset = FRotator::NormalizeAxis(RootYawOffset);
        
		// 최대 Offset 제한 (Turn 시작 전)
		if (!bIsTurningInPlace)
		{
			RootYawOffset = FMath::Clamp(RootYawOffset, -90.0f, 90.0f);
		}
        
		if (bIsTurningInPlace)
		{
			float DeltaDistanceCurve = DistanceCurve - LastDistanceCurve;
			RootYawOffset -= DeltaDistanceCurve;
            
			float AbsRootYawOffset = FMath::Abs(RootYawOffset);
			if (AbsRootYawOffset > TurnStartThreshold)
			{
				float YawExcess = AbsRootYawOffset - TurnStartThreshold;
                
				if (RootYawOffset > 0.0f)
				{
					RootYawOffset -= YawExcess;
				}
				else
				{
					RootYawOffset += YawExcess;
				}
			}
		}
	}
}

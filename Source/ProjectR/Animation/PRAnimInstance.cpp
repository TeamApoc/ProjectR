// Fill out your copyright notice in the Description page of Project Settings.

#include "PRAnimInstance.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "KismetAnimationLibrary.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/PRGameplayTags.h"
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

	if (!IsValid(PlayerCharacter))
	{
		PlayerCharacter = Cast<APRPlayerCharacter>(GetOwningActor());
	}

	if (IsValid(PlayerCharacter) && !IsValid(CharacterMovement))
	{
		CharacterMovement = PlayerCharacter->GetCharacterMovement();
	}

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

void UPRAnimInstance::NativePostEvaluateAnimation()
{
	Super::NativePostEvaluateAnimation();
}

/*~ 이동 상태 갱신 ~*/

void UPRAnimInstance::UpdateVelocity()
{
	Velocity = CharacterMovement->Velocity;
	Velocity2D = FVector(Velocity.X, Velocity.Y, 0.0f);
	XYSpeed = Velocity.Size2D();

	const FRotator ActorRotation = PlayerCharacter->GetActorRotation();
	LocalVelocity2D = ActorRotation.UnrotateVector(Velocity2D);
}

void UPRAnimInstance::UpdateAcceleration()
{
	Acceleration = CharacterMovement->GetCurrentAcceleration();

	const FRotator ActorRotation = PlayerCharacter->GetActorRotation();
	const FVector AccelerationXY = FVector(Acceleration.X, Acceleration.Y, 0.0f);
	LocalAcceleration2D = ActorRotation.UnrotateVector(AccelerationXY);
}

void UPRAnimInstance::UpdateDirection()
{
	if (XYSpeed < MoveSpeedThreshold)
	{
		return;
	}

	const FRotator ActorRotation = PlayerCharacter->GetActorRotation();
	Direction = UKismetAnimationLibrary::CalculateDirection(Velocity2D, ActorRotation);

	float TargetRefAngle = 0.0f;
	if (FMath::Abs(Direction) <= 22.5f)
	{
		TargetRefAngle = 0.0f;
	}
	else if (Direction > 22.5f && Direction <= 67.5f)
	{
		TargetRefAngle = 45.0f;
	}
	else if (Direction > 67.5f && Direction <= 112.5f)
	{
		TargetRefAngle = 90.0f;
	}
	else if (Direction > 112.5f && Direction <= 157.5f)
	{
		TargetRefAngle = 135.0f;
	}
	else if (FMath::Abs(Direction) > 157.5f)
	{
		TargetRefAngle = 180.0f;
	}
	else if (Direction < -22.5f && Direction >= -67.5f)
	{
		TargetRefAngle = -45.0f;
	}
	else if (Direction < -67.5f && Direction >= -112.5f)
	{
		TargetRefAngle = -90.0f;
	}
	else if (Direction < -112.5f && Direction >= -157.5f)
	{
		TargetRefAngle = -135.0f;
	}

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

	// DodgeAbility가 소유한 재생 상태는 AnimInstance에서 직접 관리하지 않고 ASC 태그로 관찰한다.
	if (const UPRAbilitySystemComponent* ASC = PlayerCharacter->GetPRAbilitySystemComponent())
	{
		bIsDodging = ASC->HasMatchingGameplayTag(PRGameplayTags::State_Dodging);
	}
	else
	{
		bIsDodging = false;
	}

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

	const float Threshold = (PlayerCharacter->GetWalkSpeed() + PlayerCharacter->GetJogSpeed()) * 0.5f;
	MovementMode = XYSpeed <= Threshold ? EPRMovementMode::Walking : EPRMovementMode::Jogging;
}

/*~ 회전 및 조준 상태 갱신 ~*/

void UPRAnimInstance::UpdateTurnInPlace()
{
	const float TurnYawWeight = GetCurveValue(TurnYawWeightCurveName);
	const bool bWasTurning = bIsTurningInPlace;
	bIsTurningInPlace = TurnYawWeight > 0.1f;

	if (!bWasTurning && bIsTurningInPlace)
	{
		TurnStartYaw = RootYawOffset;
	}

	if (bWasTurning && !bIsTurningInPlace)
	{
		TargetTurnAngle = EPRTurnAngle::None;
	}
}

void UPRAnimInstance::UpdateRootYawOffset()
{
	const float DeltaSeconds = GetWorld()->GetDeltaSeconds();
	const FRotator ActorRot = PlayerCharacter->GetActorRotation();
	const FRotator AimRot = PlayerCharacter->GetBaseAimRotation();
	const float CurrentDiff = UKismetMathLibrary::NormalizedDeltaRotator(AimRot, ActorRot).Yaw;

	if (bShouldMove || bIsFalling)
	{
		RootYawOffset = FMath::FInterpTo(RootYawOffset, 0.0f, DeltaSeconds, RootYawOffsetInterpSpeed);
		return;
	}

	const float TurnSwitch = GetCurveValue(TurnYawWeightCurveName);
	const float RotationAlpha = GetCurveValue(TurnRotationCurveName);
	const bool bWasTurning = bIsTurningInPlace;
	bIsTurningInPlace = TurnSwitch > 0.5f;

	if (bIsTurningInPlace)
	{
		if (!bWasTurning)
		{
			TurnStartYaw = CurrentDiff;
		}

		RootYawOffset = TurnStartYaw * (1.0f - FMath::Clamp(RotationAlpha, 0.0f, 1.0f));
	}
	else
	{
		RootYawOffset = 0.0f;
	}

	RemainingTurnYaw = RootYawOffset;
}

void UPRAnimInstance::UpdateAim()
{
	const FRotator ActorRot = PlayerCharacter->GetActorRotation();
	const FRotator AimRot = PlayerCharacter->GetBaseAimRotation();
	const float CurrentDiff = UKismetMathLibrary::NormalizedDeltaRotator(AimRot, ActorRot).Yaw;

	ArmedState = EPRArmedState::Unarmed;
	EquippedWeaponSlot = EPRWeaponSlotType::None;
	AimOffsetWeaponSlot = EPRWeaponSlotType::None;

	if (const UPRWeaponManagerComponent* WeaponManager = PlayerCharacter->GetWeaponManager())
	{
		ArmedState = WeaponManager->GetArmedState();
		EquippedWeaponSlot = WeaponManager->GetCurrentWeaponSlot();
		AimOffsetWeaponSlot = WeaponManager->GetAimOffsetWeaponSlot();
	}

	if (bIsStrafeMode)
	{
		AimYaw = CurrentDiff - RootYawOffset;
	}
	else if (FMath::Abs(CurrentDiff) < TurnThreshold - 5.0f)
	{
		AimYaw = CurrentDiff;
	}
	else
	{
		AimYaw = FMath::FInterpTo(AimYaw, 0.0f, GetWorld()->GetDeltaSeconds(), 5.0f);
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

	const FRotator ActorRot = PlayerCharacter->GetActorRotation();
	const FRotator AimRot = PlayerCharacter->GetBaseAimRotation();
	const float AbsDiff = FMath::Abs(UKismetMathLibrary::NormalizedDeltaRotator(AimRot, ActorRot).Yaw);

	TargetTurnAngle = AbsDiff > TurnThreshold ? EPRTurnAngle::Angle90 : EPRTurnAngle::None;
}

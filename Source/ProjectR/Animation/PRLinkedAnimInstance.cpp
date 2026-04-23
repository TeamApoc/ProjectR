// Fill out your copyright notice in the Description page of Project Settings.


#include "PRLinkedAnimInstance.h"

#include "PRAnimInstance.h"

void UPRLinkedAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	
	MainAnimInstance = Cast<UPRAnimInstance>(Blueprint_GetMainAnimInstance());
}

void UPRLinkedAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	
	if (!IsValid(MainAnimInstance))
	{
		return;
	}
	
	MovementMode = MainAnimInstance->MovementMode;
	bShouldMove = MainAnimInstance->bShouldMove;
	bIsFalling = MainAnimInstance->bIsFalling;
	bIsSprint = MainAnimInstance->bIsSprint;
	bIsCrouching = MainAnimInstance->bIsCrouching;
	bIsAiming = MainAnimInstance->bIsAiming;
	bIsWalking = MainAnimInstance->bIsWalking;
	F_OrientationAngle = MainAnimInstance->F_OrientationAngle;
	FR_OrientationAngle = MainAnimInstance->FR_OrientationAngle;
	FL_OrientationAngle = MainAnimInstance->FL_OrientationAngle;
	B_OrientationAngle = MainAnimInstance->B_OrientationAngle;
	BL_OrientationAngle = MainAnimInstance->BL_OrientationAngle;
	BR_OrientationAngle = MainAnimInstance->BR_OrientationAngle;
	CardinalDirection = MainAnimInstance->CardinalDirection;
	AimPitch = MainAnimInstance->AimPitch;
	AimYaw = MainAnimInstance->AimYaw;
	RootYawOffset = MainAnimInstance->RootYawOffset;
	TargetTurnAngle = MainAnimInstance->TargetTurnAngle;
	XYSpeed = MainAnimInstance->XYSpeed;
	Direction = MainAnimInstance->Direction;
}


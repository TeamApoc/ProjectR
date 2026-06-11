// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (Spring Arm 컴포넌트 구현)
#include "ProjectR/Player/Components/PRSpringArmComponent.h"

#include "ProjectR/Character/PRPlayerCharacter.h"

UPRSpringArmComponent::UPRSpringArmComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UPRSpringArmComponent::SetCameraMode(EPRCameraMode NewCameraMode)
{
	CurrentCameraMode = NewCameraMode;

	if (CurrentCameraMode == EPRCameraMode::Default)
	{
		CurrentCameraModeSettings = DefaultCameraSettings;
	}
}

void UPRSpringArmComponent::SetCameraModeSettings(EPRCameraMode CameraMode, const FPRSpringArmCameraModeSettings& NewSettings)
{
	if (CameraMode == EPRCameraMode::Default)
	{
		DefaultCameraSettings = NewSettings;
		if (CurrentCameraMode == EPRCameraMode::Default)
		{
			CurrentCameraModeSettings = DefaultCameraSettings;
		}
		return;
	}

	CurrentCameraModeSettings = NewSettings;
}

const FPRSpringArmCameraModeSettings& UPRSpringArmComponent::GetCameraModeSettings() const
{
	return GetCurrentCameraModeSettings();
}

void UPRSpringArmComponent::TickComponent(float DeltaTime, enum ELevelTick TickType,
                                          FActorComponentTickFunction* ThisTickFunction)
{
	APRPlayerCharacter* PRCharacter = Cast<APRPlayerCharacter>(GetOwner());
	if (IsValid(PRCharacter) && PRCharacter->IsLocallyControlled())
	{
		const FPRSpringArmCameraModeSettings& ModeSettings = GetCurrentCameraModeSettings();

		TargetArmLength = FMath::FInterpTo(TargetArmLength, ModeSettings.TargetArmLength, DeltaTime, ArmLengthInterpSpeed);
		TargetOffset = FMath::VInterpTo(TargetOffset, ModeSettings.TargetOffset, DeltaTime, SocketOffsetInterpSpeed);
		SocketOffset = FMath::VInterpTo(SocketOffset, ModeSettings.SocketOffset, DeltaTime, SocketOffsetInterpSpeed);
	}

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

const FPRSpringArmCameraModeSettings& UPRSpringArmComponent::GetCurrentCameraModeSettings() const
{
	switch (CurrentCameraMode)
	{
	case EPRCameraMode::Aim:
	case EPRCameraMode::WeaponZoom:
		return CurrentCameraModeSettings;
	case EPRCameraMode::Default:
	default:
		return DefaultCameraSettings;
	}
}

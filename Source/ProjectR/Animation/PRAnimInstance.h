// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "PRAnimationTypes.h"
#include "ProjectR/Weapon/Types/PRWeaponTypes.h"
#include "PRAnimInstance.generated.h"

class APRPlayerCharacter;
class UCharacterMovementComponent;

/**
 * 플레이어 메인 AnimInstance다.
 * 이동, 조준, 무장 상태처럼 AnimGraph가 상시 참조하는 값을 갱신한다.
 */
UCLASS()
class PROJECTR_API UPRAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	/** 애니메이션 인스턴스 초기화 시 캐릭터와 이동 컴포넌트 참조를 캐시한다 */
	virtual void NativeInitializeAnimation() override;

	/** 매 프레임 캐릭터 이동, 조준, 무장, 액션 태그 상태를 갱신한다 */
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	/** 평가 이후 일회성 애니메이션 요청 상태를 정리한다 */
	virtual void NativePostEvaluateAnimation() override;

private:
	void UpdateVelocity();
	void UpdateAcceleration();
	void UpdateDirection();
	void UpdateFlags();
	void UpdateMovementMode();
	void UpdateAim();
	void UpdateTurnInPlace();
	void UpdateRootYawOffset();
	void DetermineTargetTurnAngle();

public:
	/*~ 캐릭터 참조 ~*/
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<APRPlayerCharacter> PlayerCharacter;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UCharacterMovementComponent> CharacterMovement;

	/*~ 이동 상태 ~*/
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	EPRMovementMode MovementMode;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float MoveSpeedThreshold = 3.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	FVector Velocity;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	FVector Velocity2D;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	FVector Acceleration;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	FVector LocalVelocity2D;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	FVector LocalAcceleration2D;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float XYSpeed;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float CameraX;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float Direction;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	ECardinalDirection CardinalDirection;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float F_OrientationAngle;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float FR_OrientationAngle;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float FL_OrientationAngle;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float B_OrientationAngle;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float BR_OrientationAngle;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float BL_OrientationAngle;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bHasAcceleration;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bShouldMove;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsFalling;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsCrouching;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsSprint;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsAiming;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsWalking;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	ELandState LandState;

	UPROPERTY(BlueprintReadOnly, Category = "Movement|Aiming")
	float AimPitch;

	UPROPERTY(BlueprintReadOnly, Category = "Movement|Aiming")
	float AimYaw;

	UPROPERTY(BlueprintReadOnly, Category = "Turn In Place")
	bool bIsTurningInPlace;

	UPROPERTY(BlueprintReadOnly, Category = "Turn In Place")
	float RemainingTurnYaw;

	UPROPERTY(BlueprintReadOnly, Category = "Turn In Place")
	float RootYawOffset;

	UPROPERTY(EditDefaultsOnly, Category = "TurnInPlace|Config")
	float TurnStartThreshold = 180.0f;

	UPROPERTY(EditDefaultsOnly, Category = "TurnInPlace|Config")
	float TurnEndThreshold = 5.0f;

	UPROPERTY(EditDefaultsOnly, Category = "TurnInPlace|Config")
	float RootYawOffsetInterpSpeed = 10.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	EPRTurnAngle TargetTurnAngle = EPRTurnAngle::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Curve Names")
	FName TurnYawWeightCurveName = FName(TEXT("TurnYawWeight"));

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Curve Names")
	FName TurnRotationCurveName = FName(TEXT("TurnRotationAlpha"));

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsStrafeMode;

	UPROPERTY(BlueprintReadOnly, Category = "Movement|Weapon")
	EPRArmedState ArmedState = EPRArmedState::Unarmed;

	UPROPERTY(BlueprintReadOnly, Category = "Movement|Weapon")
	EPRWeaponSlotType EquippedWeaponSlot = EPRWeaponSlotType::None;

	UPROPERTY(BlueprintReadOnly, Category = "Movement|Aiming")
	EPRWeaponSlotType AimOffsetWeaponSlot = EPRWeaponSlotType::None;

	/*~ Ability 액션 관찰 상태 ~*/
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Animation|Dodge")
	bool bIsDodging = false;

private:
	float TurnStartYaw = 0.0f;
	float DistanceCurve = 0.0f;
	float LastDistanceCurve = 0.0f;
	FRotator MovingRotation = FRotator::ZeroRotator;
	FRotator LastMovingRotation = FRotator::ZeroRotator;
	const float TurnThreshold = 90.0f;
	FRotator LastCameraRotation;
};

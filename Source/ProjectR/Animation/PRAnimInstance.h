// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PRAnimationTypes.h"
#include "Animation/AnimInstance.h"
#include "PRAnimInstance.generated.h"

class UCharacterMovementComponent;
class APRPlayerCharacter;
/**
 * 
 */
UCLASS()
class PROJECTR_API UPRAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	
private:
	void UpdateVelocity();
	void UpdateAcceleration();
	void UpdateDirection();
	void UpdateFlags();
	void UpdateMovementMode();
	void UpdateAim();
	void UpdateLean();
	void UpdateTurnInPlace();
	void UpdateRootYawOffset();

public:
	/*~ 캐릭터 참조 ~*/
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<APRPlayerCharacter> PlayerCharacter;
	
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UCharacterMovementComponent> CharacterMovement;
	
	/*~ 이동 상태 (Movement) ~*/
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	EPRMovementMode MovementMode;
	
	/*~ Movements ~*/
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
	
	// 전방 기준 방향 각도
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float F_OrientationAngle;
	
	// 우측 기준 방향 각도
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float R_OrientationAngle;
	
	// 후방 기준 방향 각도
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float B_OrientationAngle;
	
	// 좌측 기준 방향 각도
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float L_OrientationAngle;
	
	// 기울어짐 방향
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float LeanDirection;
	
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
	ELandState LandState;
	
	UPROPERTY(BlueprintReadOnly, Category = "Movement|Aiming")
	float AimPitch;
	
	UPROPERTY(BlueprintReadOnly, Category = "Movement|Aiming")
	float AimYaw;
	
	UPROPERTY(EditDefaultsOnly, Category = "TurnInPlace|Config")
	FName TurnYawWeightCurveName = FName("TurnYawWeight");
	
	UPROPERTY(EditDefaultsOnly, Category = "TurnInPlace|Config")
	FName RemainingTurnYawCurveName = FName("RemainingTurnYaw");
	
	// 현재 Turn 중인지 여부 (TurnYawWeight 커브 기반)
	UPROPERTY(BlueprintReadOnly, Category = "Turn In Place")
	bool bIsTurningInPlace;

	// 남은 회전량 (-180 ~ 180)
	UPROPERTY(BlueprintReadOnly, Category = "Turn In Place")
	float RemainingTurnYaw;

	// 메시에 적용할 Root Yaw Offset
	UPROPERTY(BlueprintReadOnly, Category = "Turn In Place")
	float RootYawOffset;
	
	// Turn 시작 임계값
	UPROPERTY(EditDefaultsOnly, Category = "TurnInPlace|Config")
	float TurnStartThreshold = 90.0f;

	// Turn 종료 임계값
	UPROPERTY(EditDefaultsOnly, Category = "TurnInPlace|Config")
	float TurnEndThreshold = 5.0f;

	// Offset 보간 속도
	UPROPERTY(EditDefaultsOnly, Category = "TurnInPlace|Config")
	float RootYawOffsetInterpSpeed = 10.0f;
	
private:
	// 이전 프레임의 RemainingTurnYaw (델타 계산용)
	float PreviousRemainingTurnYaw;
	// Turn 시작 시점의 Yaw 저장
	float TurnStartYaw;
	float DistanceCurve = 0.0f;
	float LastDistanceCurve = 0.0f;
	FRotator MovingRotation = FRotator::ZeroRotator;
	FRotator LastMovingRotation = FRotator::ZeroRotator;
	
};

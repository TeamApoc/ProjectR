// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PRAnimationTypes.h"
#include "Animation/AnimInstance.h"
#include "ProjectR/Weapon/Types/PRWeaponTypes.h"
#include "PRLinkedAnimInstance.generated.h"

class UPRAnimInstance;
/**
 * 
 */
UCLASS()
class PROJECTR_API UPRLinkedAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	
protected:
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UPRAnimInstance> MainAnimInstance;
	
	UPROPERTY(BlueprintReadOnly)
	bool bIsSprint;
	
	UPROPERTY(BlueprintReadOnly)
	bool bIsCrouching;
	
	UPROPERTY(BlueprintReadOnly)
	bool bIsAiming;
	
	UPROPERTY(BlueprintReadOnly)
	bool bIsWalking;
	
	UPROPERTY(BlueprintReadOnly, Category = "Flags")
	bool bShouldMove;
	
	UPROPERTY(BlueprintReadOnly, Category = "Flags")
	bool bIsFalling;
	
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	EPRMovementMode MovementMode;
	
	// 전방 기준 방향 각도
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float F_OrientationAngle;
	
	// 우측 기준 방향 각도
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float FR_OrientationAngle;
	
	// 좌측 기준 방향 각도
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float FL_OrientationAngle;
	
	// 후방 기준 방향 각도
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float B_OrientationAngle;
	
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float BL_OrientationAngle;
	
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float BR_OrientationAngle;
	
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float LeanDirection;
	
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	ECardinalDirection CardinalDirection;
	
	UPROPERTY(BlueprintReadOnly, Category = "Movement|Aiming")
	float AimPitch;
	
	UPROPERTY(BlueprintReadOnly, Category = "Movement|Aiming")
	float AimYaw;
	
	UPROPERTY(BlueprintReadOnly, Category = "Movement|Aiming")
	float RootYawOffset;
	
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float XYSpeed;
	
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float Direction;
	
	// 레이어의 AnimGraph에서 사용할 변수
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	EPRTurnAngle TargetTurnAngle = EPRTurnAngle::None;
	
	// 현재 무장 상태
	UPROPERTY(BlueprintReadOnly, Category = "Movement|Weapon")
	EPRArmedState ArmedState = EPRArmedState::Unarmed;
	
	// 현재 장착 또는 활성화된 무기 슬롯
	UPROPERTY(BlueprintReadOnly, Category = "Movement|Weapon")
	EPRWeaponSlotType EquippedWeaponSlot = EPRWeaponSlotType::None;

	// 현재 AimOffset 선택에 사용할 무기 슬롯
	UPROPERTY(BlueprintReadOnly, Category = "Movement|Aiming")
	EPRWeaponSlotType AimOffsetWeaponSlot = EPRWeaponSlotType::None;

};

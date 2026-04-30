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
	// 레이어 애니메이션 인스턴스 초기화 시 메인 AnimInstance를 캐시한다
	virtual void NativeInitializeAnimation() override;

	// 메인 AnimInstance의 애니메이션 변수를 레이어로 복사한다
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

	// 회피 애니메이션 요청 누적 번호다
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Animation|Dodge")
	int32 DodgeRequestId = 0;

	// 이번 애니메이션 평가에서 회피 요청이 들어왔는지 나타내는 일회성 플래그다
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Animation|Dodge")
	bool bDodgeRequested = false;

	// 회피 애니메이션 상태가 진행 중인지 나타낸다
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Animation|Dodge")
	bool bIsDodging = false;

	// 재생할 회피 애니메이션 종류다
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Animation|Dodge")
	EPRDodgeAnimationType DodgeAnimationType = EPRDodgeAnimationType::ForwardRoll;

	// 입력 방향 회피로 전방 구르기를 재생하는지 나타낸다
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Animation|Dodge")
	bool bIsDodgeForwardRoll = false;

	// 무입력 회피로 뒤로 물러나기를 재생하는지 나타낸다
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Animation|Dodge")
	bool bIsDodgeBackStep = false;

	// 회피 애니메이션 요청 후 경과 시간이다
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Animation|Dodge")
	float DodgeElapsedTime = 0.0f;

	// 월드 기준 회피 방향이다
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Animation|Dodge")
	FVector DodgeWorldDirection = FVector::ForwardVector;

	// 캐릭터 로컬 기준 회피 방향이다
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Animation|Dodge")
	FVector DodgeLocalDirection = FVector::ForwardVector;
};

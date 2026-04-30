// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PRAnimationTypes.h"
#include "ProjectR/Weapon/Types/PRWeaponTypes.h"
#include "Animation/AnimInstance.h"
#include "PRAnimInstance.generated.h"

class UCharacterMovementComponent;
class APRPlayerCharacter;
class UAnimMontage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPRDodgeAnimationFinishedSignature);

/**
 * 
 */
UCLASS()
class PROJECTR_API UPRAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	// 애니메이션 인스턴스 초기화 시 캐릭터 참조를 캐시한다
	virtual void NativeInitializeAnimation() override;

	// 매 프레임 캐릭터 이동 및 조준 상태를 갱신한다
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	// 애니메이션 평가 후 일회성 요청 플래그를 정리한다
	virtual void NativePostEvaluateAnimation() override;

	// 회피 애니메이션 요청 번호와 방향 정보를 갱신한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Animation|Dodge")
	void RequestDodge(const FVector& WorldDirection, EPRDodgeAnimationType AnimationType);

	// 회피 애니메이션 완료를 알리고 회피 상태를 종료한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Animation|Dodge")
	void FinishDodge();

	// 회피 애니메이션 상태를 강제로 정리한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Animation|Dodge")
	void CancelDodge();

	// 회피 중 들어온 입력을 소비하고, 취소 가능 구간이면 몽타주와 Ability 종료 흐름을 조기 실행한다
	bool HandleDodgeInput();

	// AnimNotifyState가 여는 회피 입력 취소 가능 구간을 설정한다
	void SetDodgeInputCancelWindow(bool bCanCancel, float BlendOutTime = 0.0f);

	// 마지막 회피 종료가 입력 취소로 발생했는지 반환한다
	bool WasDodgeInputCancelRequested() const { return bDodgeInputCancelRequested; }
	
private:
	void UpdateVelocity();
	void UpdateAcceleration();
	void UpdateDirection();
	void UpdateFlags();
	void UpdateMovementMode();
	void UpdateAim();
	void UpdateTurnInPlace();
	void UpdateRootYawOffset();
	void DetermineTargetTurnAngle(); // 앵글 계산 함수
	void UpdateDodge(float DeltaSeconds);
	void ClearDodgeRequestFlags();

	// 회피 요청이 들어왔을 때 현재 무장 상태와 회피 타입에 맞는 몽타주를 재생한다
	void PlayDodgeMontage();

	// Ability 취소나 상태 종료 시 메인 AnimInstance에서 재생 중인 회피 몽타주를 정리한다
	void StopDodgeMontage(float BlendOutTime);

	// 현재 무장 슬롯과 회피 타입을 기준으로 재생할 회피 몽타주를 반환한다
	UAnimMontage* GetDodgeMontage() const;

	// 메인 몽타주가 끝났을 때 현재 회피 요청과 일치하면 Ability 종료 흐름을 이어준다
	void HandleDodgeMontageEnded(UAnimMontage* Montage, bool bInterrupted, int32 EndedDodgeRequestId);

public:
	// 회피 애니메이션이 종료될 때 발행되는 이벤트다
	UPROPERTY(BlueprintAssignable, Category = "ProjectR|Animation|Dodge")
	FPRDodgeAnimationFinishedSignature OnDodgeAnimationFinished;

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
	float FR_OrientationAngle;
	
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float FL_OrientationAngle;
	
	// 후방 기준 방향 각도
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
	float TurnStartThreshold = 180.0f;

	// Turn 종료 임계값
	UPROPERTY(EditDefaultsOnly, Category = "TurnInPlace|Config")
	float TurnEndThreshold = 5.0f;

	// Offset 보간 속도
	UPROPERTY(EditDefaultsOnly, Category = "TurnInPlace|Config")
	float RootYawOffsetInterpSpeed = 10.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	EPRTurnAngle TargetTurnAngle = EPRTurnAngle::None;
	
	// turn 중인지 판단하는 커브값
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Curve Names")
	FName TurnYawWeightCurveName = FName(TEXT("TurnYawWeight"));
	
	// turn 진행도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Curve Names")
	FName TurnRotationCurveName = FName(TEXT("TurnRotationAlpha"));
	
	// 조준중, 걷는중에는 strafe
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsStrafeMode; // (bIsAiming || bIsWalking)
	
	// 현재 무장 상태
	UPROPERTY(BlueprintReadOnly, Category = "Movement|Weapon")
	EPRArmedState ArmedState = EPRArmedState::Unarmed;
	
	// 현재 장착 또는 활성화된 무기 슬롯
	UPROPERTY(BlueprintReadOnly, Category = "Movement|Weapon")
	EPRWeaponSlotType EquippedWeaponSlot = EPRWeaponSlotType::None;
	
	// 현재 AimOffset 선택에 사용할 무장 슬롯
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

	// 회피 몽타주 끝부분의 입력 취소 가능 구간이 열려 있는지 나타낸다
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Animation|Dodge")
	bool bCanCancelDodgeByInput = false;

	// 회피 애니메이션 요청 후 경과 시간이다
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Animation|Dodge")
	float DodgeElapsedTime = 0.0f;

	// 회피 완료 노티파이가 없을 때 자동 종료할 시간이다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Animation|Dodge")
	float DodgeAutoFinishTime = 3.0f;

	// 월드 기준 회피 방향이다
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Animation|Dodge")
	FVector DodgeWorldDirection = FVector::ForwardVector;

	// 캐릭터 로컬 기준 회피 방향이다
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Animation|Dodge")
	FVector DodgeLocalDirection = FVector::ForwardVector;
	
	// 비무장 상태에서 입력 방향 회피 요청에 재생할 전방 구르기 몽타주다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Animation|Dodge|Montage")
	TObjectPtr<UAnimMontage> UnarmedForwardRollMontage;

	// 비무장 상태에서 무입력 회피 요청에 재생할 백스텝 몽타주다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Animation|Dodge|Montage")
	TObjectPtr<UAnimMontage> UnarmedBackStepMontage;

	// 주무기 장착 상태에서 입력 방향 회피 요청에 재생할 전방 구르기 몽타주다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Animation|Dodge|Montage")
	TObjectPtr<UAnimMontage> PrimaryForwardRollMontage;

	// 주무기 장착 상태에서 무입력 회피 요청에 재생할 백스텝 몽타주다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Animation|Dodge|Montage")
	TObjectPtr<UAnimMontage> PrimaryBackStepMontage;

	// 보조무기 장착 상태에서 입력 방향 회피 요청에 재생할 전방 구르기 몽타주다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Animation|Dodge|Montage")
	TObjectPtr<UAnimMontage> SecondaryForwardRollMontage;

	// 보조무기 장착 상태에서 무입력 회피 요청에 재생할 백스텝 몽타주다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Animation|Dodge|Montage")
	TObjectPtr<UAnimMontage> SecondaryBackStepMontage;

	// 회피 몽타주 재생 속도다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Animation|Dodge|Montage")
	float DodgeMontagePlayRate = 1.0f;

	// 회피가 강제 종료될 때 몽타주를 자연스럽게 멈추는 블렌드 아웃 시간이다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Animation|Dodge|Montage")
	float DodgeMontageStopBlendOutTime = 0.15f;

	// 입력 취소 구간에서 회피를 조기 종료할 때 사용할 몽타주 블렌드 아웃 시간이다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Animation|Dodge|Montage")
	float DodgeInputCancelBlendOutTime = 0.18f;

	// 몽타주 종료 델리게이트가 들어오기 전에 자동 종료 fallback이 먼저 실행되지 않도록 더해주는 여유 시간이다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Animation|Dodge|Montage")
	float DodgeMontageAutoFinishPadding = 0.15f;
	
private:
	// 현재 메인 AnimInstance에서 재생 중인 회피 몽타주다
	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveDodgeMontage;

	// 현재 재생 중인 회피 몽타주를 시작한 요청 번호다
	int32 ActiveDodgeMontageRequestId = 0;

	// Turn 시작 시점의 Yaw 저장
	float TurnStartYaw = 0.0f;
	float DistanceCurve = 0.0f;
	float LastDistanceCurve = 0.0f;
	FRotator MovingRotation = FRotator::ZeroRotator;
	FRotator LastMovingRotation = FRotator::ZeroRotator;
	const float TurnThreshold = 90.0f;
	// 이전 프레임의 카메라 회전값 (오프셋 누적용)
	FRotator LastCameraRotation;

	// 현재 회피 요청에 적용할 자동 종료 시간이다
	float CurrentDodgeAutoFinishTime = 0.0f;

	// 현재 회피 입력 취소 구간에서 사용할 몽타주 블렌드 아웃 시간이다
	float CurrentDodgeInputCancelBlendOutTime = 0.0f;

	// 마지막 회피 종료가 입력 취소 구간의 입력으로 발생했는지 나타낸다
	bool bDodgeInputCancelRequested = false;
	
};

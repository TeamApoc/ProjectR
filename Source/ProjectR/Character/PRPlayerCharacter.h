// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PRCharacterBase.h"
#include "Net/UnrealNetwork.h"
#include "PRPlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

UCLASS()
class PROJECTR_API APRPlayerCharacter : public APRCharacterBase
{
	GENERATED_BODY()

public:
	APRPlayerCharacter();

	// todo : 제자리 turn 없음, lean 없음, sprint jog walk 3단계로 분류해야함
	
	/** 멀티플레이어 변수 복제 설정 */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	/** 애니메이션 인스턴스에서 사용하는 게터 함수들 */                   
	bool IsCrouching() const { return false; } 
	bool IsSprinting() const { return true; } 
	bool IsAiming() const { return false; }       
	
	// 컨트롤 회전(카메라)과 캐릭터 정면 사이의 Yaw 차이를 반환 (Lean 및 절차적 애니메이션용)
	float GetDesiredLookDirection() const;            
	
protected:
	virtual void BeginPlay() override;
	
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	/** 입력 처리 함수 */
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

	/** 상태 변경 함수 (멀티플레이어 대응) */
	void SprintStarted();
	void SprintEnded();

	UFUNCTION(Server, Reliable)
	void Server_SetSprinting(bool bNewSprinting);

	void AimStarted();
	void AimEnded();

	UFUNCTION(Server, Reliable)
	void Server_SetAiming(bool bNewAiming);

	void CrouchPressed();
	
	/** 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	/** Enhanced Input 에셋 (블루프린트에서 할당) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> DodgeAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> CrouchAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> AimAction;

private:
	/** 복제되는 상태 변수 */
	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Locomotion", meta = (AllowPrivateAccess = "true"))
	bool bIsSprinting = false;

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Locomotion", meta = (AllowPrivateAccess = "true"))
	bool bIsAiming = false;
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PRCharacterBase.h"
#include "Net/UnrealNetwork.h"
#include "ProjectR/AbilitySystem/Data/PRAbilitySet.h"
#include "PRPlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class UPRWeaponManagerComponent;
class UPRSpringArmComponent;
struct FInputActionValue;

UCLASS()
class PROJECTR_API APRPlayerCharacter : public APRCharacterBase
{
	GENERATED_BODY()

public:
	APRPlayerCharacter();
	
	/** 멀티플레이어 변수 복제 설정 */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/*~ APawn Interface ~*/
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;

	/*~ APRCharacterBase Interface ~*/
	virtual UPRAbilitySystemComponent* GetPRAbilitySystemComponent() const override;
	
	/*~ APRPlayerCharacter Interface ~*/
	/** 애니메이션 인스턴스에서 사용하는 게터 함수들 */                   
	bool IsCrouching() const { return bIsCrouched; } 
	bool IsSprinting() const { return bIsSprinting; } 
	bool IsWalking() const { return bIsWalking; }
	float GetWalkSpeed() const { return WalkSpeed; }
	float GetJogSpeed() const { return JogSpeed; }
	float GetSprintSpeed() const { return SprintSpeed; }
	bool IsAiming() const;
	
	// ===== Component getters =====
	UPRWeaponManagerComponent* GetWeaponManager() const {return WeaponManagerComponent;}

	
protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	/*~ APRCharacterBase Interface ~*/
	virtual void HandleGameplayTagUpdated(const FGameplayTag& ChangedTag, bool bTagExists) override;
	
	/*~ APRPlayerCharacter Interface ~*/
	/** 입력 처리 함수 */
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

	/** 상태 변경 함수 (멀티플레이어 대응) */
	void SprintPressed();
	void WalkPressed();

	/** 현재 상태(질주, 조준 등)에 맞춰 MaxWalkSpeed를 업데이트한다 (클라이언트 예측용) */
	void UpdateMaxWalkSpeed();

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SetSprinting(bool bNewSprinting);
	
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SetWalking(bool bNewWalking);
	
	void HandleMovementInputTag(FGameplayTag InputTag, bool bPressed);
	
private:
    /** 질주 상태가 복제되었을 때 속도를 업데이트한다 */
    UFUNCTION()
    void OnRep_IsSprinting();

public:
	/** 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UPRSpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UPRWeaponManagerComponent> WeaponManagerComponent;

protected:
	/** Enhanced Input 에셋 (블루프린트에서 할당) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> SprintAction;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> WalkAction;

	/** 조준/느린 이동 시 속도 (cm/s) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PR|Locomotion")
	float WalkSpeed = 200.0f;

	/** 기본 조깅 속도 (cm/s) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PR|Locomotion")
	float JogSpeed = 350.0f;

	/** 질주 속도 (cm/s) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PR|Locomotion")
	float SprintSpeed = 600.0f;
	
	// 게임 시작 시 기본으로 연결할 애니메이션 레이어 (맨손 레이어)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PR|Animation")
	TSubclassOf<UAnimInstance> DefaultAnimLayerClass;
private:
	/** 복제되는 상태 변수 */
	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Locomotion", meta = (AllowPrivateAccess = "true"))
	bool bIsSprinting = false;

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Locomotion", meta = (AllowPrivateAccess = "true"))
	bool bIsAiming = false;
	
	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Locomotion", meta = (AllowPrivateAccess = "true"))
	bool bIsWalking = false;
	
	FPRAbilitySetHandles AbilitySetHandles;
};

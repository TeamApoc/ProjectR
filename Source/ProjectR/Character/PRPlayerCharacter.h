// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PRCharacterBase.h"
#include "Net/UnrealNetwork.h"
#include "ProjectR/AbilitySystem/Data/PRAbilitySet.h"
#include "ProjectR/Interaction/PRInteractionInterface.h"
#include "PRPlayerCharacter.generated.h"

class UPRCameraModifier;
class USphereComponent;
class UPRInteractableComponent;
class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class UPRWeaponManagerComponent;
class UPRSpringArmComponent;
class UPRActionInputRouterComponent;
class UPRProjectileTrajectoryPreviewComponent;
struct FInputActionValue;
struct FOnAttributeChangeData;
//무기 테스트용
class UPRWeaponDataAsset;

UCLASS()
class PROJECTR_API APRPlayerCharacter : public APRCharacterBase, public IPRInteractionInterface
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

	/*~ IPRCombatInterface ~*/
	virtual EPRTeam GetTeam() const override { return EPRTeam::Player; }

	/*~ IPRInteractionInterface ~*/
	virtual UPRInteractableComponent* GetInteractableComponent() const override { return InteractableComponent; }
	
	/*~ APRPlayerCharacter Interface ~*/
	/** 애니메이션 인스턴스에서 사용하는 게터 함수들 */                   
	bool IsCrouching() const { return bIsCrouched; } 
	bool IsSprinting() const { return bIsSprinting; } 
	bool IsWalking() const { return bIsWalking; }
	bool IsDodging() const {return bIsDodging; }
	float GetWalkSpeed() const { return WalkSpeed; }
	float GetJogSpeed() const { return JogSpeed; }
	float GetSprintSpeed() const { return SprintSpeed; }
	float GetDownSpeed() const { return DownSpeed; }
	bool IsAiming() const;
	bool IsDown() const;
	bool IsMovementBlocked() const {return bBlockMove;}
	
	/** Sprint Ability가 질주 상태를 캐릭터 이동 상태에 반영 */
	void SetSprintingFromAbility(bool bNewSprinting);
	
	// ===== Component getters =====
	
	/** 액션 입력 라우터 컴포넌트를 반환 */
	UPRActionInputRouterComponent* GetActionInputRouter() const { return ActionInputRouterComponent; }
	// TODO: UPRWeaponManagerComponent::GetAimOffsetWeaponSlot() 을 사용해야 함 (애니메이션에서 참조 시)
	UFUNCTION(BlueprintPure, Category = "PR|Weapon")
	UPRWeaponManagerComponent* GetWeaponManager() const;

	
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/*~ APRCharacterBase Interface ~*/
	virtual void HandleGameplayTagUpdated(const FGameplayTag& ChangedTag, bool bTagExists) override;
	
	/*~ APRPlayerCharacter Interface ~*/
	/** 입력 처리 함수 */
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

	/** 현재 상태(질주, 조준 등)에 맞춰 MaxWalkSpeed를 업데이트한다 (클라이언트 예측용) */
	void UpdateMaxWalkSpeed();
	
private:
	/** 상태 태그 기준으로 이동 입력이 차단되는지 반환한다 */
	bool IsMoveInputLockedByState() const;

	/** 이동속도 배율 Attribute 변경 시 MaxWalkSpeed를 갱신하도록 바인딩한다 */
	void BindMovementSpeedAttributeChange();

	/** 이동속도 배율 Attribute 변경 바인딩을 해제한다 */
	void UnbindMovementSpeedAttributeChange();

	/** 이동속도 배율 Attribute 변경을 캐릭터 이동속도에 반영한다 */
	void HandleMovementSpeedMultiplierChanged(const FOnAttributeChangeData& ChangeData);

public:
	/** 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UPRSpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	/** 몽타주 기반 액션 중 입력 차단과 스킵 요청을 중계하는 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UPRActionInputRouterComponent> ActionInputRouterComponent;

	/** 투사체 발사 예측 경로 표시 컴포넌트. 로컬 시각 효과 전용 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UPRProjectileTrajectoryPreviewComponent> ProjectileTrajectoryPreviewComponent;
	
	// 상호작용 타겟 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<UPRInteractableComponent> InteractableComponent;
	
	// 상호작용 collider
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<USphereComponent> InteractionSphere;
	
	// 감도 최신화
	UFUNCTION()
	void HandleMouseSensitivityChanged(float NewSensitivity);

protected:
	/** Enhanced Input 에셋 (블루프린트에서 할당) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

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

	/** 다운 상태 기어가기 속도 (cm/s) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PR|Locomotion")
	float DownSpeed = 50.0f;
	
	/** 카메라 감도 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PR|Camera")
	float CachedCameraSensitivity = 0.5f;

private:
	bool bIsSprinting = false;
	bool bIsAiming = false;
	bool bIsWalking = false;
	bool bIsDodging = false;
	
	bool bBlockMove = false;
	
	UPROPERTY()
	TObjectPtr<UPRCameraModifier> CrouchCameraModifier; 
};

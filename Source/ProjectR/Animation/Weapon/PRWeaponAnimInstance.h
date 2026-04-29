// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "ProjectR/Weapon/Types/PRWeaponAnimationTypes.h"
#include "PRWeaponAnimInstance.generated.h"

class APRWeaponActor;
class USkeletalMeshComponent;

/**
 * 무기 스켈레탈 메시 ABP가 공통으로 읽는 애니메이션 상태 계약이다.
 */
UCLASS(Abstract, Blueprintable)
class PROJECTR_API UPRWeaponAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	/*~ UAnimInstance Interface ~*/
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativePostEvaluateAnimation() override;

public:
	// 무기 애니메이션 상태를 Idle로 되돌린다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Weapon|Animation")
	void SetIdleState();

	// 발사 애니메이션 요청 번호를 갱신하고 Shoot 상태로 전환한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Weapon|Animation")
	void RequestShoot();

	// 재장전 애니메이션 요청 번호를 갱신하고 Reload 상태로 전환한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Weapon|Animation")
	void RequestReload();

	// 지정한 액션이 아직 진행 중일 때만 Idle 상태로 복귀한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Weapon|Animation")
	void FinishAction(EPRWeaponAnimationState ExpectedState);

	// 현재 무기 애니메이션 상태가 지정 상태와 같은지 확인한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|Weapon|Animation")
	bool IsAnimationState(EPRWeaponAnimationState InState) const;

protected:
	// 무기 Actor와 메시 컴포넌트 참조를 다시 캐시한다
	void RefreshOwnerReferences();

	// 액션 상태 진입 시 경과 시간을 초기화한다
	void ResetActionTimer();

	// 이번 평가에서 소비된 일회성 애니메이션 요청 플래그를 초기화한다
	void ClearRequestFlags();

public:
	// 이 AnimInstance가 붙어 있는 무기 Actor다
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Weapon|Animation")
	TObjectPtr<APRWeaponActor> WeaponActor;

	// 이 AnimInstance가 구동하는 무기 스켈레탈 메시 컴포넌트다
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Weapon|Animation")
	TObjectPtr<USkeletalMeshComponent> WeaponMeshComponent;

	// 현재 무기 애니메이션 상태다
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Weapon|Animation")
	EPRWeaponAnimationState AnimationState = EPRWeaponAnimationState::Idle;

	// 발사 애니메이션 요청 누적 번호다
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Weapon|Animation")
	int32 ShootRequestId = 0;

	// 재장전 애니메이션 요청 누적 번호다
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Weapon|Animation")
	int32 ReloadRequestId = 0;

	// 이번 애니메이션 평가에서 발사 요청이 들어왔는지 나타내는 일회성 플래그다
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Weapon|Animation")
	bool bShootRequested = false;

	// 이번 애니메이션 평가에서 재장전 요청이 들어왔는지 나타내는 일회성 플래그다
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Weapon|Animation")
	bool bReloadRequested = false;

	// 현재 액션 상태로 전환된 뒤 지난 시간이다
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Weapon|Animation")
	float ActionElapsedTime = 0.0f;
};

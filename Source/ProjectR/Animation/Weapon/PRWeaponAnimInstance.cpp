// Copyright ProjectR. All Rights Reserved.

#include "PRWeaponAnimInstance.h"

#include "Components/SkeletalMeshComponent.h"
#include "ProjectR/Weapon/Actors/PRWeaponActor.h"

void UPRWeaponAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	// AnimInstance 초기화 시점에 무기 Actor와 메시 컴포넌트를 캐시한다.
	RefreshOwnerReferences();
	SetIdleState();
}

void UPRWeaponAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!IsValid(WeaponActor) || !IsValid(WeaponMeshComponent))
	{
		// 초기화 순서나 메시 재생성으로 참조가 비어 있으면 다시 찾는다.
		RefreshOwnerReferences();
	}

	if (AnimationState == EPRWeaponAnimationState::Idle)
	{
		// Idle 상태에서는 액션 경과 시간을 항상 0으로 유지한다.
		ActionElapsedTime = 0.0f;
		return;
	}

	// Shoot, Reload 같은 액션 상태에서만 경과 시간을 누적한다.
	ActionElapsedTime += DeltaSeconds;
}

void UPRWeaponAnimInstance::NativePostEvaluateAnimation()
{
	Super::NativePostEvaluateAnimation();

	ClearRequestFlags();
}


void UPRWeaponAnimInstance::SetIdleState()
{
	AnimationState = EPRWeaponAnimationState::Idle;
	ResetActionTimer();
}

void UPRWeaponAnimInstance::RequestShoot()
{
	// 같은 Shoot 상태 안에서도 재요청을 감지할 수 있도록 요청 번호와 펄스를 함께 갱신한다.
	AnimationState = EPRWeaponAnimationState::Shoot;
	++ShootRequestId;
	bShootRequested = true;
	ResetActionTimer();
}

void UPRWeaponAnimInstance::RequestReload()
{
	// 재장전은 발사와 별도 요청 번호를 사용해 AnimBP 전환 조건을 단순하게 유지한다.
	AnimationState = EPRWeaponAnimationState::Reload;
	++ReloadRequestId;
	bReloadRequested = true;
	ResetActionTimer();
}

void UPRWeaponAnimInstance::FinishAction(EPRWeaponAnimationState ExpectedState)
{
	if (AnimationState != ExpectedState)
	{
		// 늦게 도착한 이전 액션 종료 요청이 현재 액션을 덮어쓰지 않도록 막는다.
		return;
	}

	SetIdleState();
}

bool UPRWeaponAnimInstance::IsAnimationState(EPRWeaponAnimationState InState) const
{
	return AnimationState == InState;
}


void UPRWeaponAnimInstance::RefreshOwnerReferences()
{
	WeaponMeshComponent = GetSkelMeshComponent();
	WeaponActor = Cast<APRWeaponActor>(GetOwningActor());
}

void UPRWeaponAnimInstance::ResetActionTimer()
{
	ActionElapsedTime = 0.0f;
}

void UPRWeaponAnimInstance::ClearRequestFlags()
{
	// 요청 펄스는 AnimGraph가 한 번 평가한 뒤 자동으로 내려간다.
	bShootRequested = false;
	bReloadRequested = false;
}

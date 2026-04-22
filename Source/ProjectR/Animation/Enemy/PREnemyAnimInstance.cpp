// Copyright ProjectR. All Rights Reserved.

#include "PREnemyAnimInstance.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "ProjectR/Character/Enemy/PREnemyBaseCharacter.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/PRGameplayTags.h"

void UPREnemyAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	RefreshOwnerReferences();
}

void UPREnemyAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!IsValid(EnemyCharacter))
	{
		RefreshOwnerReferences();
	}

	if (!IsValid(EnemyCharacter))
	{
		return;
	}

	UpdateMovementData();
	UpdateStateFlags();
}

void UPREnemyAnimInstance::RefreshOwnerReferences()
{
	// AnimInstance는 초기화 시점에 Pawn이 아직 없을 수 있어 Update에서도 다시 캐시한다.
	EnemyCharacter = Cast<APREnemyBaseCharacter>(TryGetPawnOwner());
	CharacterMovement = IsValid(EnemyCharacter) ? EnemyCharacter->GetCharacterMovement() : nullptr;
}

void UPREnemyAnimInstance::UpdateMovementData()
{
	if (!IsValid(EnemyCharacter) || !IsValid(CharacterMovement))
	{
		Velocity = FVector::ZeroVector;
		XYSpeed = 0.0f;
		bShouldMove = false;
		bIsFalling = false;
		return;
	}

	Velocity = EnemyCharacter->GetVelocity();
	XYSpeed = Velocity.Size2D();
	// ABP에서는 이 bool만 보고 Idle/Move를 나누면 된다.
	bShouldMove = XYSpeed > MoveSpeedThreshold;
	bIsFalling = CharacterMovement->IsFalling();
}

void UPREnemyAnimInstance::UpdateStateFlags()
{
	bIsDead = false;
	bIsGroggy = false;

	if (!IsValid(EnemyCharacter))
	{
		return;
	}

	if (UPRAbilitySystemComponent* ASC = EnemyCharacter->GetPRAbilitySystemComponent())
	{
		// 상태 전환은 애니메이션 자체 판단이 아니라 ASC 태그를 신뢰한다.
		bIsDead = ASC->HasMatchingGameplayTag(PRGameplayTags::State_Dead);
		bIsGroggy = ASC->HasMatchingGameplayTag(PRGameplayTags::State_Groggy);
	}
}

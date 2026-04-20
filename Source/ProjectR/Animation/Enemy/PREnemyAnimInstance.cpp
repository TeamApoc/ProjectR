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
		bIsDead = ASC->HasMatchingGameplayTag(PRGameplayTags::State_Dead);
		bIsGroggy = ASC->HasMatchingGameplayTag(PRGameplayTags::State_Groggy);
	}
}

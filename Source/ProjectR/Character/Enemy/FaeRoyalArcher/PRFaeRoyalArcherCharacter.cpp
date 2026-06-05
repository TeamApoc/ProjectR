// Copyright ProjectR. All Rights Reserved.

#include "PRFaeRoyalArcherCharacter.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "ProjectR/AI/Data/PRFaeRoyalArcherCombatDataAsset.h"

APRFaeRoyalArcherCharacter::APRFaeRoyalArcherCharacter()
{
	// 스탯 Registry / DT_EnemyStats의 RowName과 일치해야 한다.
	CharacterID = TEXT("Fae_RoyalArcher");

	ApplyRoyalArcherMovementDefaults();
}

void APRFaeRoyalArcherCharacter::BeginPlay()
{
	Super::BeginPlay();

	ApplyRoyalArcherMovementDefaults();

	if (bStartInFlyingMode && IsValid(GetCharacterMovement()))
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	}
}

UPRFaeRoyalArcherCombatDataAsset* APRFaeRoyalArcherCharacter::GetRoyalArcherCombatData() const
{
	return Cast<UPRFaeRoyalArcherCombatDataAsset>(GetCombatDataAsset());
}

void APRFaeRoyalArcherCharacter::ApplyRoyalArcherMovementDefaults()
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!IsValid(MovementComponent))
	{
		return;
	}

	MovementComponent->DefaultLandMovementMode = MOVE_Flying;
	MovementComponent->MaxFlySpeed = DefaultMaxFlySpeed;
	MovementComponent->BrakingDecelerationFlying = DefaultBrakingDecelerationFlying;
	MovementComponent->RotationRate = FRotator(0.0f, DefaultFlightRotationYawRate, 0.0f);
	MovementComponent->bOrientRotationToMovement = false;
	MovementComponent->bUseControllerDesiredRotation = true;
	bUseControllerRotationYaw = false;
}

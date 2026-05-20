// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#include "PRPenitentCharacter.h"

#include "GameFramework/CharacterMovementComponent.h"

APRPenitentCharacter::APRPenitentCharacter()
{
	// 스탯 Registry / DT_EnemyStats의 RowName과 맞아야 한다.
	CharacterID = TEXT("Penitent");

	GetCharacterMovement()->MaxWalkSpeed = 260.0f;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 420.0f, 0.0f);
}

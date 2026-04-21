// Copyright ProjectR. All Rights Reserved.

#include "PRSoldierArmoredCharacter.h"

#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

APRSoldierArmoredCharacter::APRSoldierArmoredCharacter()
{
	CharacterID = TEXT("Soldier_Armored");

	GetCharacterMovement()->MaxWalkSpeed = 360.0f;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 420.0f, 0.0f);

	if (IsValid(ArmorCollision))
	{
		ArmorCollision->SetBoxExtent(FVector(42.0f, 36.0f, 78.0f));
		ArmorCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		ArmorCollision->SetCollisionObjectType(ECC_Pawn);
		ArmorCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
		ArmorCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		ArmorCollision->ComponentTags.Reset();
		ArmorCollision->ComponentTags.Add(TEXT("Armor.Torso"));
	}

	if (IsValid(WeakpointCollision))
	{
		WeakpointCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		WeakpointCollision->ComponentTags.Reset();
	}
}

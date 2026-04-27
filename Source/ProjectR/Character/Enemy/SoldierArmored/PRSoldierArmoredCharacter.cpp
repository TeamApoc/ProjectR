// Copyright ProjectR. All Rights Reserved.

#include "PRSoldierArmoredCharacter.h"

#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MotionWarpingComponent.h"
#include "ProjectR/AI/Components/PRSoldierArmoredDebugDrawComponent.h"

APRSoldierArmoredCharacter::APRSoldierArmoredCharacter()
{
	DebugDrawComponent = CreateDefaultSubobject<UPRSoldierArmoredDebugDrawComponent>(TEXT("DebugDrawComponent"));
	MotionWarpingComponent = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarpingComponent"));

	// 스탯 Registry / DT_EnemyStats의 RowName과 맞아야 한다.
	CharacterID = TEXT("Soldier_Armored");

	GetCharacterMovement()->MaxWalkSpeed = 360.0f;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 420.0f, 0.0f);

	if (IsValid(ArmorCollision))
	{
		// Soldier_Armored는 몸통 갑옷 판정만 우선 사용한다.
		// 실제 위치/크기는 BP에서 메시 비율에 맞춰 다시 조정할 수 있다.
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
		// 현재 Soldier_Armored 설계에는 별도 약점 판정이 없으므로 비활성화한다.
		WeakpointCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		WeakpointCollision->ComponentTags.Reset();
	}
}

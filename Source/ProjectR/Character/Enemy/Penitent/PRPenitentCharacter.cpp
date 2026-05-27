// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#include "PRPenitentCharacter.h"

#include "ProjectR/AbilitySystem/Abilities/Enemy/Penitent/PRPenitentBarrierActor.h"

APRPenitentCharacter::APRPenitentCharacter()
{
	// 스탯 Registry / DT_EnemyStats의 RowName과 일치
	CharacterID = TEXT("Penitent");
	
	BarrierAttachPoint = CreateDefaultSubobject<USceneComponent>(TEXT("BarrierAttachPoint"));
	BarrierAttachPoint->SetupAttachment(RootComponent);
	BarrierAttachPoint->SetRelativeLocation(FVector(120.0f, 0.0f, 0.0f));
	BarrierAttachPoint->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	
}

void APRPenitentCharacter::SetSpawnedBarrierActor(APRPenitentBarrierActor* InBarrierActor)
{
	SpawnedBarrierActor = InBarrierActor;
}

void APRPenitentCharacter::ClearSpawnedBarrierActor(APRPenitentBarrierActor* BarrierActorToClear)
{
	if (IsValid(BarrierActorToClear) && SpawnedBarrierActor != BarrierActorToClear)
	{
		return;
	}

	SpawnedBarrierActor = nullptr;
}

bool APRPenitentCharacter::HasActiveBarrier() const
{
	return IsValid(SpawnedBarrierActor);
}

// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#include "PRPenitentCharacter.h"

#include "ProjectR/Actors/Enemy/PRPenitentBarrierActor.h"

APRPenitentCharacter::APRPenitentCharacter()
{
	// 스탯 Registry / DT_EnemyStats의 RowName과 맞아야 한다.
	CharacterID = TEXT("Penitent");
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

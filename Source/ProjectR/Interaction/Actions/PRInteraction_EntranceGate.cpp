// Copyright ProjectR. All Rights Reserved.

#include "PRInteraction_EntranceGate.h"

#include "GameFramework/PlayerState.h"
#include "ProjectR/Player/PRPlayerController.h"
#include "ProjectR/World/PRSpawnPointTags.h"

UPRInteraction_EntranceGate::UPRInteraction_EntranceGate()
{
	// 입장 게이트 전원 상호작용 기본 설정
}

void UPRInteraction_EntranceGate::HandlePartySyncConditionMet()
{
	for (APlayerState* PlayerState : GetPlayerArray())
	{
		APRPlayerController* Controller = Cast<APRPlayerController>(PlayerState->GetOwner());
		if (!IsValid(Controller))
		{
			continue;
		}

		Controller->ClientNotifyMapTransition(FadeDuration, EPRMapTransitionType::MapTravel);
	}
	
	// 입장 게이트 전용 고정 목적지 이동
	StartTravelToSpawnPoint(TargetMap, ResolveTargetSpawnPointId());
}

FGameplayTag UPRInteraction_EntranceGate::ResolveTargetSpawnPointId() const
{
	// 기본 SpawnPoint 식별자
	return TargetSpawnPointId.IsValid() ? TargetSpawnPointId : PRSpawnPointTags::SpawnPoint_Default;
}

// Copyright ProjectR. All Rights Reserved.

#include "PRWaypointActor.h"

#include "PRWaypointTags.h"
#include "GameFramework/PlayerStart.h"

APRWaypointActor::APRWaypointActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

FGameplayTag APRWaypointActor::GetWaypointId() const
{
	return WaypointId.IsValid() ? WaypointId : PRWaypointTags::Waypoint_Default;
}

bool APRWaypointActor::MatchesWaypointId(FGameplayTag InWaypointId) const
{
	// 빈 태그 보정
	const FGameplayTag TargetWaypointId = InWaypointId.IsValid() ? InWaypointId : PRWaypointTags::Waypoint_Default;
	return GetWaypointId().MatchesTagExact(TargetWaypointId);
}

APlayerStart* APRWaypointActor::GetLinkedPlayerStart(int32 PlayerIndex) const
{
	if (LinkedPlayerStarts.IsValidIndex(PlayerIndex) && IsValid(LinkedPlayerStarts[PlayerIndex].Get()))
	{
		return LinkedPlayerStarts[PlayerIndex].Get();
	}

	// Fallback
	for (const TObjectPtr<APlayerStart>& LinkedPlayerStart : LinkedPlayerStarts)
	{
		if (IsValid(LinkedPlayerStart.Get()))
		{
			// 첫 유효 PlayerStart 대체
			return LinkedPlayerStart.Get();
		}
	}

	return nullptr;
}

FTransform APRWaypointActor::GetSpawnTransform(int32 PlayerIndex) const
{
	if (APlayerStart* LinkedPlayerStart = GetLinkedPlayerStart(PlayerIndex))
	{
		return LinkedPlayerStart->GetActorTransform();
	}

	// PlayerStart 미연결 대체
	UE_LOG(LogTemp, Warning, TEXT("Waypoint %s has no linked PlayerStart. Using waypoint transform."), *GetName());
	return GetActorTransform();
}

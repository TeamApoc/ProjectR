// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "PRInteractableActor.h"
#include "PRWaypointActor.generated.h"

class APlayerStart;

UCLASS()
class PROJECTR_API APRWaypointActor : public APRInteractableActor
{
	GENERATED_BODY()

public:
	APRWaypointActor();

public:
	// Waypoint 식별 태그 반환
	FGameplayTag GetWaypointId() const;

	// 지정 태그와 Waypoint 식별자 일치 여부 반환
	bool MatchesWaypointId(FGameplayTag InWaypointId) const;

	// 플레이어 인덱스 기준 연결 PlayerStart 반환
	APlayerStart* GetLinkedPlayerStart(int32 PlayerIndex) const;

	// 플레이어 인덱스 기준 스폰 Transform 반환
	FTransform GetSpawnTransform(int32 PlayerIndex) const;

protected:
	// Waypoint 식별 태그
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "Waypoint"),  Category = "ProjectR|Waypoint")
	FGameplayTag WaypointId;

	// 플레이어 인덱스별 레벨 에디터 연결 PlayerStart
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Waypoint")
	TArray<TObjectPtr<APlayerStart>> LinkedPlayerStarts;
};

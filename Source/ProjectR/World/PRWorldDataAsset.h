// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/UI/WaypointTravel/PRWaypointTravelTypes.h"
#include "PRWorldDataAsset.generated.h"

// Travel UI에 표시할 맵과 웨이포인트 목적지 목록
UCLASS(BlueprintType)
class PROJECTR_API UPRWorldDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// 지정한 웨이포인트 ID를 가진 목적지 노드 존재 여부 반환
	bool HasWaypointNode(FGameplayTag WaypointId) const
	{
		return WaypointNodes.ContainsByPredicate([WaypointId](const FPRWaypointTravelNodeDefinition& Node)
		{
			return Node.WaypointId.MatchesTagExact(WaypointId);
		});
	}

public:
	// ServerTravel 대상 맵
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|WaypointTravel")
	TSoftObjectPtr<UWorld> MapAsset;

	// UI에 표시할 맵 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|WaypointTravel")
	FText DisplayName;

	// 이 맵에서 선택 가능한 웨이포인트 노드 목록
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|WaypointTravel")
	TArray<FPRWaypointTravelNodeDefinition> WaypointNodes;
};

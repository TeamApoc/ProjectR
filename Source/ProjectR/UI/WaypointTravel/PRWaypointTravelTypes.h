// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "PRWaypointTravelTypes.generated.h"

class UPRWorldDataAsset;

// Travel UI에서 하나의 버튼이 가리키는 목적지 웨이포인트
USTRUCT(BlueprintType)
struct PROJECTR_API FPRWaypointTravelNodeDefinition
{
	GENERATED_BODY()

	// 목적지 맵의 SpawnPoint와 매칭할 웨이포인트 식별자
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|WaypointTravel", meta = (Categories = "SpawnPoint"))
	FGameplayTag WaypointId;

	// UI에 표시할 웨이포인트 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|WaypointTravel")
	FText DisplayName;
};

// Travel UI와 서버 요청 사이에서 전달하는 목적지 노드
USTRUCT(BlueprintType)
struct PROJECTR_API FPRWaypointTravelNodeOption
{
	GENERATED_BODY()

	// 목적지 월드 데이터 에셋
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|WaypointTravel")
	TSoftObjectPtr<UPRWorldDataAsset> WorldDataAsset;

	// ServerTravel 대상 맵
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|WaypointTravel")
	TSoftObjectPtr<UWorld> MapAsset;

	// 목적지 맵의 SpawnPoint와 매칭할 웨이포인트 식별자
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|WaypointTravel", meta = (Categories = "SpawnPoint"))
	FGameplayTag WaypointId;

	// UI에 표시할 맵 이름
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|WaypointTravel")
	FText WorldDisplayName;

	// UI에 표시할 웨이포인트 이름
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|WaypointTravel")
	FText WaypointDisplayName;
};

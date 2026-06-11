// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (Entrance 보스 입장 게이트 상호작용 액션 실행 로직 구현)
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "PRInteraction_MapTravelBase.h"
#include "PRInteraction_EntranceGate.generated.h"

class UWorld;

// 고정 맵과 고정 SpawnPoint로 이동하는 입장 게이트 상호작용
UCLASS()
class PROJECTR_API UPRInteraction_EntranceGate : public UPRInteraction_MapTravelBase
{
	GENERATED_BODY()

public:
	UPRInteraction_EntranceGate();

protected:
	/*~ UPRInteraction_PartySyncBase Interface ~*/
	// 입장 게이트 조건 충족 시 고정 목적지 이동 처리
	virtual void HandlePartySyncConditionMet() override;

	/*~ UPRInteraction_EntranceGate Interface ~*/
	// 입장 게이트 목적지 SpawnPoint 태그 반환
	FGameplayTag ResolveTargetSpawnPointId() const;

protected:
	// 입장 게이트 이동 대상 맵
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Interaction|EntranceGate")
	TSoftObjectPtr<UWorld> TargetMap;

	// 입장 게이트 목적지 SpawnPoint 태그
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Interaction|EntranceGate")
	FGameplayTag TargetSpawnPointId;
	
	// 페이드 아웃 시간
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Interaction|EntranceGate")
	float FadeDuration = 1.6f;
	
};

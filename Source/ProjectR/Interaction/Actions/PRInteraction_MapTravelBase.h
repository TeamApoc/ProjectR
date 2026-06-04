// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "TimerManager.h"
#include "ProjectR/Interaction/Actions/PRInteraction_PartySyncBase.h"
#include "PRInteraction_MapTravelBase.generated.h"

class UWorld;

// 파티 동기화 완료 이후 ServerTravel 실행 기반 클래스
UCLASS(Abstract)
class PROJECTR_API UPRInteraction_MapTravelBase : public UPRInteraction_PartySyncBase
{
	GENERATED_BODY()

public:
	// 맵 이동 상호작용 기본값 설정
	UPRInteraction_MapTravelBase();

protected:
	/*~ UPRInteraction_PartySyncBase Interface ~*/
	// 맵 이동 진행 중 파티 동기화 중복 처리 차단 여부
	virtual bool IsPartySyncActionLocked() const override;

	// 하위 클래스가 제공한 맵과 SpawnPoint 태그로 ServerTravel 실행
	void StartTravelToSpawnPoint(TSoftObjectPtr<UWorld> MapToTravel, FGameplayTag SpawnPointId);

private:
	// 페이드아웃 이후 ServerTravel 예약 타이머
	FTimerHandle TravelDelayTimerHandle;

	// 맵 이동 예약 이후 중복 ServerTravel 요청을 막기 위한 진행 상태
	bool bTravelInProgress = false;
};

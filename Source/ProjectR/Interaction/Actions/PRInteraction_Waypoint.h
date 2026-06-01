// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PRInteraction_MapTravelBase.h"
#include "PRInteraction_Waypoint.generated.h"

UCLASS()
class PROJECTR_API UPRInteraction_Waypoint : public UPRInteraction_MapTravelBase
{
	GENERATED_BODY()

public:
	UPRInteraction_Waypoint();

protected:
	// Waypoint 이동 성립 시 활성 Waypoint와 체크포인트 상태 갱신
	virtual void OnTravelConditionMet() override;

	// Waypoint 상호작용 시작 이벤트 전송
	virtual void NotifyTravelInteractionStarted(AActor* Interactor) override;

	// Waypoint 상호작용 종료 이벤트 전송
	virtual void NotifyTravelInteractionEnded(AActor* Interactor, bool bCanceled) override;
};

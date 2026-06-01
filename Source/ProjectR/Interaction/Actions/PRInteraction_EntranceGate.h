// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PRInteraction_MapTravelBase.h"
#include "PRInteraction_EntranceGate.generated.h"

UCLASS()
class PROJECTR_API UPRInteraction_EntranceGate : public UPRInteraction_MapTravelBase
{
	GENERATED_BODY()

public:
	UPRInteraction_EntranceGate();

protected:
	virtual void NotifyTravelInteractionStarted(AActor* Interactor) override;
	virtual void NotifyTravelInteractionEnded(AActor* Interactor, bool bCanceled) override;
	
	// 입장 게이트 이동 성립 시 진행 상태를 기록하지 않는 후크
	virtual void OnTravelConditionMet() override;
	
};

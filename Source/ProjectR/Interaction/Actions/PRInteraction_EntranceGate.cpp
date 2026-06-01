// Copyright ProjectR. All Rights Reserved.

#include "PRInteraction_EntranceGate.h"

UPRInteraction_EntranceGate::UPRInteraction_EntranceGate()
{
	InteractionType = EPRInteractionType::Sustained;
	bRequiresRange = true;
}

void UPRInteraction_EntranceGate::NotifyTravelInteractionStarted(AActor* Interactor)
{
	Super::NotifyTravelInteractionStarted(Interactor);
}

void UPRInteraction_EntranceGate::NotifyTravelInteractionEnded(AActor* Interactor, bool bCanceled)
{
	Super::NotifyTravelInteractionEnded(Interactor, bCanceled);
}

void UPRInteraction_EntranceGate::OnTravelConditionMet()
{
	// 입장 게이트는 체크포인트와 활성 Waypoint 상태를 기록하지 않음
}

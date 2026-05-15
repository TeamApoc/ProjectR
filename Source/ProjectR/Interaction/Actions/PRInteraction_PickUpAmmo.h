// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/Interaction/PRInteractionAction.h"
#include "PRInteraction_PickUpAmmo.generated.h"

/**
 * 픽업 액터의 탄약량을 상호작용한 액터의 슬롯 예비탄에 적립한다.
 * 캡 초과로 일부만 흡수되면 픽업이 잔여량을 유지한 채 월드에 남는다.
 */
UCLASS()
class PROJECTR_API UPRInteraction_PickUpAmmo : public UPRInteractionAction
{
	GENERATED_BODY()

public:
	virtual void Execute_Implementation(AActor* Interactor) override;
};

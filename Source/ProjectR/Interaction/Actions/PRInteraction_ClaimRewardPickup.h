// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/Interaction/PRInteractionAction.h"
#include "PRInteraction_ClaimRewardPickup.generated.h"

// 보상 픽업 입력을 드롭 매니저로 전달하는 즉발 상호작용이다
UCLASS()
class PROJECTR_API UPRInteraction_ClaimRewardPickup : public UPRInteractionAction
{
	GENERATED_BODY()

public:
	UPRInteraction_ClaimRewardPickup();

	/*~ UPRInteractionAction Interface ~*/
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual void Execute_Implementation(AActor* Interactor) override;
};

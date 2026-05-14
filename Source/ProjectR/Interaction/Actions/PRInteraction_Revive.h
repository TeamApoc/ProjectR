// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/Interaction/PRInteractionAction.h"
#include "PRInteraction_Revive.generated.h"

class UPRConsumableDataAsset;
/**
 * 
 */
UCLASS()
class PROJECTR_API UPRInteraction_Revive : public UPRInteractionAction
{
	GENERATED_BODY()
	
public:
	virtual void OnHoldStart_Implementation(AActor* Interactor) override;
	virtual void OnHoldCanceled_Implementation(AActor* Interactor) override;
	virtual void OnHoldFinished_Implementation(AActor* Interactor) override;
	virtual void Execute_Implementation(AActor* Interactor) override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	
private:
	void FinishReviveAbility(AActor* Interactor);
	
public:
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	TObjectPtr<UPRConsumableDataAsset> CostItem;
};

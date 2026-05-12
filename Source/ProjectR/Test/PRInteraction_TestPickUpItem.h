// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/Interaction/PRInteractionAction.h"
#include "PRInteraction_TestPickUpItem.generated.h"

class UPRItemDataAsset;
/**
 * 
 */
UCLASS()
class PROJECTR_API UPRInteraction_TestPickUpItem : public UPRInteractionAction
{
	GENERATED_BODY()
	
public:
	virtual void Execute_Implementation(AActor* Interactor) override;
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Inventory")
	TObjectPtr<UPRItemDataAsset> ItemData;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Inventory")
	int32 Amount;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Inventory")
	bool bDestroyOnPickUp = true;
};

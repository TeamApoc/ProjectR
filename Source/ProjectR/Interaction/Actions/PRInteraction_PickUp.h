// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/Interaction/PRInteractionAction.h"
#include "PRInteraction_PickUp.generated.h"

class UPRWeaponDataAsset;
/**
 * 
 */
UCLASS()
class PROJECTR_API UPRInteraction_PickUp : public UPRInteractionAction
{
	GENERATED_BODY()
	
public:
	virtual void Execute_Implementation(AActor* Interactor) override;
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<UPRWeaponDataAsset> WeaponDataAsset; // 테스트용 무기 데이터, 추후 Consumable로 변경
};

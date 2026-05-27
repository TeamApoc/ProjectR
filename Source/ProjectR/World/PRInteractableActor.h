// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectR/Interaction/PRInteractionInterface.h"
#include "ProjectR/UI/WorldMarker/PRPingMarkerTargetInterface.h"
#include "PRInteractableActor.generated.h"

UCLASS()
class PROJECTR_API APRInteractableActor : public AActor, public IPRInteractionInterface, public IPRPingMarkerTargetInterface
{
	GENERATED_BODY()

public:
	APRInteractableActor();
	
	/*~ IPRInteractionInterface ~*/
	virtual UPRInteractableComponent* GetInteractableComponent() const override {return InteractableComponent;}
	
	/*~ IPRPingMarkerTargetInterface ~*/
	virtual FPRWorldMarkerVisualData GetPingMarkerVisualData_Implementation() const override;
	virtual FVector GetPingMarkerWorldLocation_Implementation() const override;
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<UPRInteractableComponent> InteractableComponent;
	
	// ===== Configs =====
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|UI")
	FVector PingMarkerOffset = FVector(0.0f, 0.0f, 30.0f);
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|UI")
	bool bOverridesPingMarker = false;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "bOverridesPingMarker == true" ),  Category = "ProjectR|UI")
	FPRWorldMarkerVisualData PingMarkerVisualOverride;
};

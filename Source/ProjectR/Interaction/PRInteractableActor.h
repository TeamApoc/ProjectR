// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PRInteractionInterface.h"
#include "GameFramework/Actor.h"
#include "PRInteractableActor.generated.h"

UCLASS()
class PROJECTR_API APRInteractableActor : public AActor, public IPRInteractionInterface
{
	GENERATED_BODY()

public:
	APRInteractableActor();
	
	virtual UPRInteractableComponent* GetInteractableComponent() const override {return InteractableComponent;}
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<UPRInteractableComponent> InteractableComponent;
};

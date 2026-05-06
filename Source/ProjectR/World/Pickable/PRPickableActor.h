// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/Interaction/PRInteractableActor.h"
#include "PRPickableActor.generated.h"

class USphereComponent;

UCLASS()
class PROJECTR_API APRPickableActor : public APRInteractableActor
{
	GENERATED_BODY()

public:
	APRPickableActor();
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<USphereComponent> InteractionCollision;
};

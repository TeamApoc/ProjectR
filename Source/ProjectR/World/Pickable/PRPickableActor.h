// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/UI/WorldMarker/PRPingMarkerTargetInterface.h"
#include "ProjectR/World/PRInteractableActor.h"
#include "PRPickableActor.generated.h"

class USphereComponent;

UCLASS()
class PROJECTR_API APRPickableActor : public APRInteractableActor
{
	GENERATED_BODY()

public:
	APRPickableActor();

protected:
	// ===== Components =====
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|UI")
	TObjectPtr<USphereComponent> InteractionCollision;
};

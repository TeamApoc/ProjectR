// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRInteractableActor.h"

#include "PRInteractableComponent.h"


// Sets default values
APRInteractableActor::APRInteractableActor()
{
	PrimaryActorTick.bCanEverTick = false;
	InteractableComponent = CreateDefaultSubobject<UPRInteractableComponent>(TEXT("InteractableComponent"));
	bReplicates = true;
}

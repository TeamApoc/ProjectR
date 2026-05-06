// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRPickableActor.h"

#include "Components/SphereComponent.h"
#include "ProjectR/Interaction/PRInteractableComponent.h"
#include "ProjectR/Interaction/Actions/PRInteraction_PickUp.h"

APRPickableActor::APRPickableActor()
{
	PrimaryActorTick.bCanEverTick = false;
	
	InteractionCollision = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionCollision"));
	InteractionCollision->SetSphereRadius(30.f);
	InteractionCollision->SetCollisionProfileName(FName("Interactable"));
	InteractionCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetRootComponent(InteractionCollision);
	
	InteractableComponent->InteractionActions.Add(CreateDefaultSubobject<UPRInteraction_PickUp>(TEXT("PickUpAction")));
}

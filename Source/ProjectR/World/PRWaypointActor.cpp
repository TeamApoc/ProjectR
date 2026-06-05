// Copyright ProjectR. All Rights Reserved.

#include "PRWaypointActor.h"

#include "Components/SceneComponent.h"
#include "ProjectR/Interaction/PRInteractableComponent.h"
#include "ProjectR/System/PRDeveloperSettings.h"

APRWaypointActor::APRWaypointActor()
{
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	InteractableComponent = CreateDefaultSubobject<UPRInteractableComponent>(TEXT("InteractableComponent"));
	bReplicates = true;
}

FPRWorldMarkerVisualData APRWaypointActor::GetPingMarkerVisualData_Implementation() const
{
	if (bOverridesPingMarker)
	{
		return PingMarkerVisualOverride;
	}

	if (const UPRDeveloperSettings* Settings = GetDefault<UPRDeveloperSettings>())
	{
		return Settings->GetWorldMarkerPreset(EPRWorldMarkerPreset::Interactable);
	}

	return FPRWorldMarkerVisualData();
}

FVector APRWaypointActor::GetPingMarkerWorldLocation_Implementation() const
{
	return GetActorLocation() + PingMarkerOffset;
}

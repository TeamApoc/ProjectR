// Copyright ProjectR. All Rights Reserved.

#include "PRInteraction_ClaimRewardPickup.h"

#include "ProjectR/World/Drop/PRItemDropManagerSubsystem.h"
#include "ProjectR/World/Pickable/PRRewardPickupActor.h"

UPRInteraction_ClaimRewardPickup::UPRInteraction_ClaimRewardPickup()
{
	InteractionType = EPRInteractionType::Instant;
	TriggerType = EPRTriggerType::Tap;
	bRequiresRange = true;
	bShowHint = false;
	ActionName = NSLOCTEXT("ProjectR", "ClaimRewardPickupActionName", "획득");
	ActionHintText = NSLOCTEXT("ProjectR", "ClaimRewardPickupActionHint", "획득");
}

bool UPRInteraction_ClaimRewardPickup::CanInteract_Implementation(AActor* Interactor) const
{
	const APRRewardPickupActor* RewardPickup = Cast<APRRewardPickupActor>(GetOwner());
	return IsValid(RewardPickup) && RewardPickup->CanBeClaimedBy(Interactor);
}

void UPRInteraction_ClaimRewardPickup::Execute_Implementation(AActor* Interactor)
{
	Super::Execute_Implementation(Interactor);

	APRRewardPickupActor* RewardPickup = Cast<APRRewardPickupActor>(GetOwner());
	if (!IsValid(RewardPickup) || !RewardPickup->HasAuthority())
	{
		return;
	}

	UWorld* World = RewardPickup->GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	UPRItemDropManagerSubsystem* DropManager = World->GetSubsystem<UPRItemDropManagerSubsystem>();
	if (!IsValid(DropManager))
	{
		return;
	}

	DropManager->ClaimPickup(RewardPickup, Interactor);
}

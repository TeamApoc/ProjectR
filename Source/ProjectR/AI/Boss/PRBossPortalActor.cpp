// Copyright ProjectR. All Rights Reserved.

#include "PRBossPortalActor.h"

#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

APRBossPortalActor::APRBossPortalActor()
{
	PatternLifeSpan = 0.0f;
}

void APRBossPortalActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APRBossPortalActor, bPortalActive);
}

void APRBossPortalActor::StartPortalTelegraph()
{
	if (!HasAuthority())
	{
		return;
	}

	MulticastPortalTelegraphStarted();

	if (ActivationDelay <= 0.0f)
	{
		ActivatePortal();
		return;
	}

	GetWorldTimerManager().SetTimer(
		PortalActivationTimerHandle,
		this,
		&APRBossPortalActor::ActivatePortal,
		ActivationDelay,
		false);
}

void APRBossPortalActor::ActivatePortal()
{
	if (!HasAuthority() || bPortalActive)
	{
		return;
	}

	bPortalActive = true;
	MulticastPortalActivated();

	if (ActiveDuration <= 0.0f)
	{
		ExpirePortal();
		return;
	}

	GetWorldTimerManager().SetTimer(
		PortalExpireTimerHandle,
		this,
		&APRBossPortalActor::ExpirePortal,
		ActiveDuration,
		false);
}

void APRBossPortalActor::ExpirePortal()
{
	if (!HasAuthority())
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(PortalActivationTimerHandle);
	GetWorldTimerManager().ClearTimer(PortalExpireTimerHandle);

	bPortalActive = false;
	MulticastPortalExpired();

	if (bDestroyWhenExpired)
	{
		ExpirePatternActor();
	}
}

void APRBossPortalActor::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority() && bAutoStartPortal)
	{
		StartPortalTelegraph();
	}
}

void APRBossPortalActor::MulticastPortalTelegraphStarted_Implementation()
{
	BP_OnPortalTelegraphStarted();
}

void APRBossPortalActor::MulticastPortalActivated_Implementation()
{
	BP_OnPortalActivated();
}

void APRBossPortalActor::MulticastPortalExpired_Implementation()
{
	BP_OnPortalExpired();
}

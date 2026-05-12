// Copyright ProjectR. All Rights Reserved.

#include "PRFaerinCharacter.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "ProjectR/Game/PRPlayHUD.h"
#include "ProjectR/UI/HUD/PRHUDWidget.h"
#include "TimerManager.h"

APRFaerinCharacter::APRFaerinCharacter()
{
	PhaseThresholdRatios.Add(EPRBossPhase::Phase2, 0.87f);
	PhaseThresholdRatios.Add(EPRBossPhase::Phase3, 0.65f);
	PhaseThresholdRatios.Add(EPRBossPhase::Phase4, 0.25f);
}

/*~ AActor Interface ~*/

void APRFaerinCharacter::BeginPlay()
{
	Super::BeginPlay();

	CurrentBossHealthBarBindRetryCount = 0;
	bBossHealthBarBound = false;
	TryBindBossHealthBar();
}

void APRFaerinCharacter::TryBindBossHealthBar()
{
	if (bBossHealthBarBound)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PlayerController = It->Get();
		if (!IsValid(PlayerController) || !PlayerController->IsLocalController())
		{
			continue;
		}

		APRPlayHUD* PlayHUD = Cast<APRPlayHUD>(PlayerController->GetHUD());
		if (!IsValid(PlayHUD))
		{
			continue;
		}

		UPRHUDWidget* HUDWidget = PlayHUD->GetHUDWidget();
		if (!IsValid(HUDWidget))
		{
			continue;
		}

		HUDWidget->BindBossHealthBar(this);
		bBossHealthBarBound = true;
		World->GetTimerManager().ClearTimer(BossHealthBarBindRetryTimerHandle);
		return;
	}

	HandleBossHealthBarBindRetry();
}

void APRFaerinCharacter::HandleBossHealthBarBindRetry()
{
	if (bBossHealthBarBound || CurrentBossHealthBarBindRetryCount >= MaxBossHealthBarBindRetryCount)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World) || World->GetTimerManager().IsTimerActive(BossHealthBarBindRetryTimerHandle))
	{
		return;
	}

	++CurrentBossHealthBarBindRetryCount;
	World->GetTimerManager().SetTimer(
		BossHealthBarBindRetryTimerHandle,
		this,
		&APRFaerinCharacter::TryBindBossHealthBar,
		BossHealthBarBindRetryInterval,
		false);
}

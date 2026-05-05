// Copyright ProjectR. All Rights Reserved.

#include "PRBossPatternActor.h"

#include "Components/SceneComponent.h"
#include "Net/UnrealNetwork.h"
#include "ProjectR/Character/Enemy/PRBossBaseCharacter.h"

APRBossPatternActor::APRBossPatternActor()
{
	bReplicates = true;
	SetReplicateMovement(true);
	SetNetUpdateFrequency(30.0f);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}

void APRBossPatternActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APRBossPatternActor, OwnerBoss);
	DOREPLIFETIME(APRBossPatternActor, PatternTarget);
	DOREPLIFETIME(APRBossPatternActor, bPatternActorInitialized);
}

void APRBossPatternActor::InitializeBossPatternActor(APRBossBaseCharacter* InOwnerBoss, AActor* InPatternTarget)
{
	if (!HasAuthority() || bPatternActorInitialized)
	{
		return;
	}

	OwnerBoss = InOwnerBoss;
	PatternTarget = InPatternTarget;

	if (IsValid(OwnerBoss))
	{
		SetOwner(OwnerBoss);
		SetInstigator(OwnerBoss);
	}

	bPatternActorInitialized = true;
	BP_OnPatternActorInitialized();
}

void APRBossPatternActor::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority() && PatternLifeSpan > 0.0f)
	{
		SetLifeSpan(PatternLifeSpan);
	}
}

void APRBossPatternActor::OnRep_PatternActorInitialized()
{
	if (bPatternActorInitialized)
	{
		BP_OnPatternActorInitialized();
	}
}

void APRBossPatternActor::ExpirePatternActor()
{
	BP_OnPatternActorExpired();

	if (HasAuthority())
	{
		Destroy();
	}
}

// Copyright ProjectR. All Rights Reserved.

#include "PRFaerinCharacter.h"

#include "Engine/World.h"
#include "ProjectR/AI/Components/PRFaerinCombatDirectorComponent.h"
#include "ProjectR/AI/Components/PRFaerinDebugDrawComponent.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/System/PREventManagerSubsystem.h"

APRFaerinCharacter::APRFaerinCharacter()
{
	CombatDirectorComponent = CreateDefaultSubobject<UPRFaerinCombatDirectorComponent>(TEXT("CombatDirectorComponent"));
	DebugDrawComponent = CreateDefaultSubobject<UPRFaerinDebugDrawComponent>(TEXT("DebugDrawComponent"));

	PhaseThresholdRatios.Add(EPRBossPhase::Phase2, 0.87f);
	PhaseThresholdRatios.Add(EPRBossPhase::Phase3, 0.65f);
	PhaseThresholdRatios.Add(EPRBossPhase::Phase4, 0.25f);
}

/*~ AActor Interface ~*/

void APRFaerinCharacter::BeginPlay()
{
	Super::BeginPlay();

	BroadcastBossEncounterBegin();
}

void APRFaerinCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	BroadcastBossEncounterEnd();

	Super::EndPlay(EndPlayReason);
}

/*~ 보스 조우 이벤트 브로드캐스트 ~*/

void APRFaerinCharacter::BroadcastBossEncounterBegin()
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	UPREventManagerSubsystem* EventMgr = World->GetSubsystem<UPREventManagerSubsystem>();
	if (!IsValid(EventMgr))
	{
		return;
	}

	FPRBossEncounterEventPayload Payload;
	Payload.Boss = this;
	EventMgr->BroadcastTyped(PRGameplayTags::Event_Boss_Encounter_Begin, Payload);
}

void APRFaerinCharacter::BroadcastBossEncounterEnd()
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	UPREventManagerSubsystem* EventMgr = World->GetSubsystem<UPREventManagerSubsystem>();
	if (!IsValid(EventMgr))
	{
		return;
	}

	EventMgr->BroadcastEmpty(PRGameplayTags::Event_Boss_Encounter_End);
}

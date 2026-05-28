// Copyright ProjectR. All Rights Reserved.

#include "PRFaerinCharacter.h"

#include "Engine/World.h"
#include "MotionWarpingComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "ProjectR/AI/Components/PRFaerinCombatDirectorComponent.h"
#include "ProjectR/AI/Components/PRFaerinDebugDrawComponent.h"
#include "ProjectR/AI/Components/PRFaerinGodFallComponent.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/System/PREventManagerSubsystem.h"

APRFaerinCharacter::APRFaerinCharacter()
{
	CombatDirectorComponent = CreateDefaultSubobject<UPRFaerinCombatDirectorComponent>(TEXT("CombatDirectorComponent"));
	DebugDrawComponent = CreateDefaultSubobject<UPRFaerinDebugDrawComponent>(TEXT("DebugDrawComponent"));
	GodFallComponent = CreateDefaultSubobject<UPRFaerinGodFallComponent>(TEXT("GodFallComponent"));
	MotionWarpingComponent = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarpingComponent"));

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

void APRFaerinCharacter::Multicast_SpawnNearTeleportBodyNiagara_Implementation(
	UNiagaraSystem* NiagaraSystem,
	FName AttachSocketName)
{
	if (!IsValid(NiagaraSystem) || !IsValid(GetMesh()))
	{
		return;
	}

	const FTransform SpawnTransform = AttachSocketName != NAME_None
		? GetMesh()->GetSocketTransform(AttachSocketName)
		: GetMesh()->GetComponentTransform();
	UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		this,
		NiagaraSystem,
		SpawnTransform.GetLocation(),
		SpawnTransform.Rotator(),
		SpawnTransform.GetScale3D(),
		true);
}

void APRFaerinCharacter::Multicast_SetNearTeleportHidden_Implementation(bool bShouldHide)
{
	SetActorHiddenInGame(bShouldHide);
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

// Copyright ProjectR. All Rights Reserved.

#include "PRFaerinCharacter.h"

#include "Engine/World.h"
#include "MotionWarpingComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "ProjectR/AI/Components/PRFaerinCombatDirectorComponent.h"
#include "ProjectR/AI/Components/PRFaerinDebugDrawComponent.h"
#include "ProjectR/AI/Components/PRFaerinGodFallComponent.h"
#include "ProjectR/AI/Components/PRFaerinTeleportVFXComponent.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/System/PREventManagerSubsystem.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

APRFaerinCharacter::APRFaerinCharacter()
{
	CombatDirectorComponent = CreateDefaultSubobject<UPRFaerinCombatDirectorComponent>(TEXT("CombatDirectorComponent"));
	DebugDrawComponent = CreateDefaultSubobject<UPRFaerinDebugDrawComponent>(TEXT("DebugDrawComponent"));
	GodFallComponent = CreateDefaultSubobject<UPRFaerinGodFallComponent>(TEXT("GodFallComponent"));
	TeleportVFXComponent = CreateDefaultSubobject<UPRFaerinTeleportVFXComponent>(TEXT("TeleportVFXComponent"));
	MotionWarpingComponent = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarpingComponent"));

	PhaseThresholdRatios.Add(EPRBossPhase::Phase2, 0.87f);
	PhaseThresholdRatios.Add(EPRBossPhase::Phase3, 0.65f);
	PhaseThresholdRatios.Add(EPRBossPhase::Phase4, 0.25f);
}

/*~ AActor Interface ~*/

void APRFaerinCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		SetBossEncounterActive(true);
	}
}

void APRFaerinCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HasAuthority())
	{
		SetBossEncounterActive(false);
	}
	else if (bBossEncounterEventBroadcasted)
	{
		BroadcastBossEncounterEnd();
		bBossEncounterEventBroadcasted = false;
	}

	Super::EndPlay(EndPlayReason);
}

void APRFaerinCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APRFaerinCharacter, bBossEncounterActive);
}

void APRFaerinCharacter::Multicast_SpawnNearTeleportBodyNiagara_Implementation(
	UNiagaraSystem* NiagaraSystem,
	FName AttachSocketName)
{
	SpawnNearTeleportBodyNiagaraLocal(NiagaraSystem, AttachSocketName);
}

void APRFaerinCharacter::Multicast_SpawnFaerinWorldNiagara_Implementation(
	FSoftObjectPath NiagaraSystemPath,
	FVector_NetQuantize Location,
	FRotator Rotation,
	FVector Scale,
	float LifeSeconds)
{
	SpawnFaerinWorldNiagaraLocal(NiagaraSystemPath, Location, Rotation, Scale, LifeSeconds);
}

void APRFaerinCharacter::Multicast_PlayNearTeleportReappearPresentation_Implementation(
	FVector ReappearLocation,
	FRotator ReappearRotation,
	UNiagaraSystem* TeleportOutNiagaraSystem,
	UNiagaraSystem* ReappearDissolveNiagaraSystem,
	FName AttachSocketName)
{
	SetActorLocationAndRotation(ReappearLocation, ReappearRotation, false, nullptr, ETeleportType::TeleportPhysics);
	SetActorHiddenInGame(false);
	SpawnNearTeleportBodyNiagaraLocal(TeleportOutNiagaraSystem, AttachSocketName);
	SpawnNearTeleportBodyNiagaraLocal(ReappearDissolveNiagaraSystem, AttachSocketName);
}

void APRFaerinCharacter::Multicast_SetNearTeleportHidden_Implementation(bool bShouldHide)
{
	SetActorHiddenInGame(bShouldHide);
}

void APRFaerinCharacter::SpawnNearTeleportBodyNiagaraLocal(
	UNiagaraSystem* NiagaraSystem,
	FName AttachSocketName) const
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

void APRFaerinCharacter::SpawnFaerinWorldNiagaraLocal(const FSoftObjectPath& NiagaraSystemPath,
	const FVector& Location,
	const FRotator& Rotation,
	const FVector& Scale,
	const float LifeSeconds) const
{
	if (!NiagaraSystemPath.IsValid())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	UNiagaraSystem* NiagaraSystem = Cast<UNiagaraSystem>(NiagaraSystemPath.ResolveObject());
	if (!IsValid(NiagaraSystem))
	{
		NiagaraSystem = Cast<UNiagaraSystem>(NiagaraSystemPath.TryLoad());
	}

	if (!IsValid(NiagaraSystem))
	{
		return;
	}

	UNiagaraComponent* NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		World,
		NiagaraSystem,
		Location,
		Rotation,
		Scale,
		true,
		true);
	if (!IsValid(NiagaraComponent) || LifeSeconds <= UE_SMALL_NUMBER)
	{
		return;
	}

	TWeakObjectPtr<UNiagaraComponent> WeakNiagaraComponent = NiagaraComponent;
	FTimerHandle CleanupTimerHandle;
	World->GetTimerManager().SetTimer(
		CleanupTimerHandle,
		FTimerDelegate::CreateWeakLambda(this, [WeakNiagaraComponent]()
		{
			if (UNiagaraComponent* ActiveNiagaraComponent = WeakNiagaraComponent.Get())
			{
				ActiveNiagaraComponent->Deactivate();
				ActiveNiagaraComponent->DestroyComponent();
			}
		}),
		LifeSeconds,
		false);
}

/*~ 보스 조우 이벤트 브로드캐스트 ~*/

void APRFaerinCharacter::SetBossEncounterActive(bool bActive)
{
	if (!HasAuthority() || bBossEncounterActive == bActive)
	{
		return;
	}

	bBossEncounterActive = bActive;
	HandleBossEncounterActiveChanged();
	ForceNetUpdate();
}

void APRFaerinCharacter::HandleBossEncounterActiveChanged()
{
	if (bBossEncounterActive)
	{
		if (!bBossEncounterEventBroadcasted)
		{
			BroadcastBossEncounterBegin();
			bBossEncounterEventBroadcasted = true;
		}
	}
	else if (bBossEncounterEventBroadcasted)
	{
		BroadcastBossEncounterEnd();
		bBossEncounterEventBroadcasted = false;
	}
}

void APRFaerinCharacter::OnRep_BossEncounterActive()
{
	HandleBossEncounterActiveChanged();
}

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

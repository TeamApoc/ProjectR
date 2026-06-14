// Copyright ProjectR. All Rights Reserved.

#include "PRFaerinEncounterDirector.h"

#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "MovieScene.h"
#include "MovieSceneSequencePlaybackSettings.h"
#include "MovieSceneSequencePlayer.h"
#include "Net/UnrealNetwork.h"
#include "ProjectR/AI/Boss/PRBossSpawnProviderInterface.h"
#include "ProjectR/AI/Components/PREnemyThreatComponent.h"
#include "ProjectR/Character/Enemy/Boss/PRFaerinCharacter.h"
#include "ProjectR/Character/Enemy/PRBossBaseCharacter.h"
#include "ProjectR/Character/Enemy/PREnemyInterface.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/Game/PRGameStateBase.h"
#include "ProjectR/Game/PRPlayGameMode.h"
#include "ProjectR/Player/PRPlayerController.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "ProjectR/World/PRFaerinEncounterBoundaryActor.h"

APRFaerinEncounterDirector::APRFaerinEncounterDirector()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
}

/*~ AActor Interface ~*/

void APRFaerinEncounterDirector::BeginPlay()
{
	Super::BeginPlay();

	if (bPrimeIntroStartPoseOnBeginPlay)
	{
		PrimeIntroStartPoseLocal();
	}

	if (bApplyIntroStartLoopBeforeIntro)
	{
		ApplyIntroStartLoopAnimationsLocal();
	}

	if (!HasAuthority())
	{
		return;
	}

	if (APRPlayGameMode* GameMode = GetWorld()->GetAuthGameMode<APRPlayGameMode>())
	{
		GameMode->OnPartyWipeConfirmed.AddDynamic(this, &ThisClass::HandlePartyWipeConfirmed);
		GameMode->OnPartyRespawned.AddDynamic(this, &ThisClass::HandlePartyRespawned);
	}
}

void APRFaerinEncounterDirector::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HasAuthority())
	{
		CloseChoiceUIForInstigator();
	}

	if (HasAuthority())
	{
		if (APRPlayGameMode* GameMode = GetWorld()->GetAuthGameMode<APRPlayGameMode>())
		{
			GameMode->OnPartyWipeConfirmed.RemoveDynamic(this, &ThisClass::HandlePartyWipeConfirmed);
			GameMode->OnPartyRespawned.RemoveDynamic(this, &ThisClass::HandlePartyRespawned);
		}
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(IntroSequenceTimerHandle);
		World->GetTimerManager().ClearTimer(FightStartSequenceTimerHandle);
	}

	StopLocalSequences();
	Super::EndPlay(EndPlayReason);
}

void APRFaerinEncounterDirector::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APRFaerinEncounterDirector, CurrentState);
}

// ===== 공개 함수 =====

void APRFaerinEncounterDirector::NotifyPlayerEnteredGather(APRPlayerCharacter* Player)
{
	if (!HasAuthority() || !IsEligibleEncounterPlayer(Player))
	{
		return;
	}

	if (CurrentState == EFaerinEncounterState::Combat)
	{
		NotifyPlayerEnteredCombatArena(Player);
		return;
	}

	if (CurrentState != EFaerinEncounterState::PreIntro)
	{
		return;
	}

	if (IntroRequiredPlayers.Num() == 0)
	{
		BuildIntroRequiredPlayers();
	}

	AddPlayer(IntroEnteredPlayers, Player);
	if (AreAllIntroRequiredPlayersEntered())
	{
		StartIntroSequence();
	}
}

void APRFaerinEncounterDirector::NotifyPlayerEnteredCombatArena(APRPlayerCharacter* Player)
{
	if (!HasAuthority() || CurrentState != EFaerinEncounterState::Combat || !IsEligibleEncounterPlayer(Player))
	{
		return;
	}

	const bool bAlreadyTracked = ContainsPlayer(CombatStartParticipants, Player);
	AddPlayer(CombatStartParticipants, Player);

	if (bAlreadyTracked)
	{
		return;
	}

	TArray<APRPlayerCharacter*> LatePlayers;
	LatePlayers.Add(Player);
	AddThreatToCombatBoss(LatePlayers, Player, LateJoinThreat);
}

bool APRFaerinEncounterDirector::CanOpenChoiceDialogue(APRPlayerCharacter* Player) const
{
	if (!IsEligibleEncounterPlayer(Player))
	{
		return false;
	}

	const bool bChoiceState =
		CurrentState == EFaerinEncounterState::NegotiationIdle ||
		CurrentState == EFaerinEncounterState::Declined;
	if (!bChoiceState)
	{
		return false;
	}

	if (IsValid(BoundaryActor)
		&& !BoundaryActor->IsPlayerInsideArena(Player)
		&& !BoundaryActor->IsPlayerPhysicallyInsideArena(Player))
	{
		return false;
	}

	return true;
}

bool APRFaerinEncounterDirector::CanShowChoiceDialoguePrompt(APRPlayerCharacter* Player) const
{
	if (!IsEligibleEncounterPlayer(Player))
	{
		return false;
	}

	const bool bChoiceState =
		CurrentState == EFaerinEncounterState::NegotiationIdle ||
		CurrentState == EFaerinEncounterState::Declined;
	if (!bChoiceState)
	{
		return false;
	}

	if (IsValid(BoundaryActor) && !BoundaryActor->IsPlayerPhysicallyInsideArena(Player))
	{
		return false;
	}

	return true;
}

void APRFaerinEncounterDirector::StartChoiceDialogue(APRPlayerCharacter* Player)
{
	if (!HasAuthority() || !CanOpenChoiceDialogue(Player))
	{
		return;
	}

	ChoiceInstigator = Player;
	SetEncounterState(EFaerinEncounterState::ChoiceDialogue);
	OpenChoiceUIForPlayer(Player);
}

void APRFaerinEncounterDirector::ChooseFight(APRPlayerCharacter* Player)
{
	if (!HasAuthority() || CurrentState != EFaerinEncounterState::ChoiceDialogue)
	{
		return;
	}

	if (IsValid(ChoiceInstigator) && ChoiceInstigator != Player)
	{
		return;
	}

	if (!IsEligibleEncounterPlayer(Player))
	{
		return;
	}

	CloseChoiceUIForInstigator();
	SnapshotCombatStartParticipants(Player);

	TArray<APRPlayerCharacter*> Participants;
	GetPlayersFromSet(CombatStartParticipants, Participants);
	if (Participants.Num() == 0)
	{
		return;
	}

	if (IsValid(BoundaryActor))
	{
		BoundaryActor->SetBoundaryMode(EFaerinBoundaryMode::PreFight);
	}

	SetInputLockedForPlayers(Participants, true);
	AlignCombatStartParticipants();
	StartFightSequence();
}

void APRFaerinEncounterDirector::ChooseDecline(APRPlayerCharacter* Player)
{
	if (!HasAuthority() || CurrentState != EFaerinEncounterState::ChoiceDialogue)
	{
		return;
	}

	if (IsValid(ChoiceInstigator) && ChoiceInstigator != Player)
	{
		return;
	}

	ChoiceInstigator = Player;
	CloseChoiceUIForInstigator();

	TArray<APRPlayerCharacter*> Participants;
	GetPlayersFromSet(CombatStartParticipants, Participants);
	SetInputLockedForPlayers(Participants, false);
	MulticastSetPresentationActorsHidden(false);
	MulticastApplyNegotiationPresentationIdle();
	if (IsValid(BoundaryActor))
	{
		BoundaryActor->SetBoundaryMode(EFaerinBoundaryMode::Negotiation);
	}

	SetEncounterState(EFaerinEncounterState::Declined);
	SetEncounterState(EFaerinEncounterState::NegotiationIdle);
}

void APRFaerinEncounterDirector::HandlePartyWipeConfirmed()
{
	if (!HasAuthority())
	{
		return;
	}

	if (CurrentState != EFaerinEncounterState::Combat && CurrentState != EFaerinEncounterState::CombatStarting)
	{
		return;
	}

	CloseChoiceUIForInstigator();
	SetEncounterState(EFaerinEncounterState::WipeReset);
	ResetCombatBoss();
}

void APRFaerinEncounterDirector::HandlePartyRespawned()
{
	if (!HasAuthority())
	{
		return;
	}

	if (CurrentState != EFaerinEncounterState::WipeReset && CurrentState != EFaerinEncounterState::Combat)
	{
		return;
	}

	RestoreNegotiationAfterWipe();
}

void APRFaerinEncounterDirector::MarkDefeated()
{
	if (!HasAuthority())
	{
		return;
	}

	bCombatBossDefeated = true;
	UnbindCombatBossDefeat();
	CloseChoiceUIForInstigator();
	SetEncounterState(EFaerinEncounterState::Defeated);

	if (IsValid(BoundaryActor))
	{
		BoundaryActor->SetBoundaryMode(EFaerinBoundaryMode::Defeated);
		BoundaryActor->ClearEncounterState();
	}

	MulticastSetPresentationActorsHidden(true);
}

// ===== 복제/멀티캐스트 =====

void APRFaerinEncounterDirector::HandleDialogueNodePresentedLocal(const FPRFaerinDialogueNode& Node)
{
	if (Node.NodeType == EPRFaerinDialogueNodeType::StageShot && !Node.CameraShotId.IsNone())
	{
		ReceiveDialogueStageShotChanged(Node.CameraShotId);
	}

	if (Node.NodeType == EPRFaerinDialogueNodeType::Dialog)
	{
		if (Node.VoiceEventPath.IsValid())
		{
			ReceiveDialogueVoiceEvent(Node.VoiceEventPath);
		}

		if (!Node.EmoteId.IsNone() || !Node.EmoteObjectPath.IsEmpty())
		{
			ReceiveDialogueEmoteChanged(Node.EmoteId, Node.EmoteObjectPath);
		}
	}

	ReceiveDialogueNodePresented(Node);
}

void APRFaerinEncounterDirector::OnRep_CurrentState(EFaerinEncounterState PreviousState)
{
	ReceiveEncounterStateChanged(PreviousState, CurrentState);
}

void APRFaerinEncounterDirector::HandleSpawnedCombatBossDestroyed(AActor* DestroyedActor)
{
	if (DestroyedActor != SpawnedCombatBoss.Get())
	{
		return;
	}

	UnbindCombatBossDefeat();
	SpawnedCombatBoss = nullptr;

	if (bResettingCombatBoss || bCombatBossDefeated || CurrentState != EFaerinEncounterState::Combat)
	{
		return;
	}

	bCombatBossDefeated = true;
	MarkDefeated();
}

void APRFaerinEncounterDirector::MulticastPlayEncounterSequence_Implementation(EFaerinEncounterSequence SequenceType)
{
	PlaySequenceLocal(SequenceType);
}

void APRFaerinEncounterDirector::MulticastSetPresentationActorsHidden_Implementation(bool bPresentationHidden)
{
	for (AActor* PresentationActor : PresentationActors)
	{
		if (IsValid(PresentationActor))
		{
			PresentationActor->SetActorHiddenInGame(bPresentationHidden);
			PresentationActor->SetActorEnableCollision(false);
			PresentationActor->SetActorTickEnabled(!bPresentationHidden);
		}
	}
}

// ===== 상태/플레이어 관리 =====

void APRFaerinEncounterDirector::MulticastApplyNegotiationPresentationIdle_Implementation()
{
	ApplyNegotiationPresentationIdleLocal();
}

void APRFaerinEncounterDirector::MulticastPrimeIntroStartPose_Implementation()
{
	PrimeIntroStartPoseLocal();
}

void APRFaerinEncounterDirector::MulticastApplyIntroStartLoopAnimations_Implementation()
{
	ApplyIntroStartLoopAnimationsLocal();
}

bool APRFaerinEncounterDirector::IsEligibleEncounterPlayer(APRPlayerCharacter* Player) const
{
	if (!IsValid(Player))
	{
		return false;
	}

	const APRPlayerState* PlayerState = Player->GetPlayerState<APRPlayerState>();
	if (!IsValid(PlayerState))
	{
		return false;
	}

	return PlayerState->IsCombatParticipant() && !PlayerState->IsOutOfFight();
}

bool APRFaerinEncounterDirector::ContainsPlayer(
	const TSet<TWeakObjectPtr<APRPlayerCharacter>>& Players,
	APRPlayerCharacter* Player) const
{
	if (!IsValid(Player))
	{
		return false;
	}

	for (const TWeakObjectPtr<APRPlayerCharacter>& WeakPlayer : Players)
	{
		if (WeakPlayer.Get() == Player)
		{
			return true;
		}
	}

	return false;
}

void APRFaerinEncounterDirector::AddPlayer(
	TSet<TWeakObjectPtr<APRPlayerCharacter>>& Players,
	APRPlayerCharacter* Player)
{
	if (IsValid(Player))
	{
		Players.Add(Player);
	}
}

void APRFaerinEncounterDirector::CleanupPlayerSet(TSet<TWeakObjectPtr<APRPlayerCharacter>>& Players) const
{
	for (auto It = Players.CreateIterator(); It; ++It)
	{
		if (!It->IsValid())
		{
			It.RemoveCurrent();
		}
	}
}

void APRFaerinEncounterDirector::SetEncounterState(EFaerinEncounterState NewState)
{
	if (!HasAuthority() || CurrentState == NewState)
	{
		return;
	}

	const EFaerinEncounterState PreviousState = CurrentState;
	CurrentState = NewState;
	ReceiveEncounterStateChanged(PreviousState, CurrentState);
	ForceNetUpdate();
}

void APRFaerinEncounterDirector::BuildIntroRequiredPlayers()
{
	IntroRequiredPlayers.Reset();

	const UWorld* World = GetWorld();
	const APRGameStateBase* GameState = IsValid(World) ? World->GetGameState<APRGameStateBase>() : nullptr;
	if (!IsValid(GameState))
	{
		return;
	}

	for (APRPlayerCharacter* Player : GameState->GetPlayerCharacters())
	{
		if (IsEligibleEncounterPlayer(Player))
		{
			AddPlayer(IntroRequiredPlayers, Player);
		}
	}
}

bool APRFaerinEncounterDirector::AreAllIntroRequiredPlayersEntered() const
{
	if (IntroRequiredPlayers.Num() == 0)
	{
		return false;
	}

	for (const TWeakObjectPtr<APRPlayerCharacter>& WeakPlayer : IntroRequiredPlayers)
	{
		APRPlayerCharacter* Player = WeakPlayer.Get();
		if (!IsValid(Player) || !ContainsPlayer(IntroEnteredPlayers, Player))
		{
			return false;
		}
	}

	return true;
}

void APRFaerinEncounterDirector::GetPlayersFromSet(
	const TSet<TWeakObjectPtr<APRPlayerCharacter>>& Players,
	TArray<APRPlayerCharacter*>& OutPlayers) const
{
	OutPlayers.Reset();
	for (const TWeakObjectPtr<APRPlayerCharacter>& WeakPlayer : Players)
	{
		if (APRPlayerCharacter* Player = WeakPlayer.Get())
		{
			OutPlayers.Add(Player);
		}
	}
}

// ===== 인트로/전투 시작 흐름 =====

void APRFaerinEncounterDirector::StartIntroSequence()
{
	TArray<APRPlayerCharacter*> IntroPlayers;
	GetPlayersFromSet(IntroEnteredPlayers, IntroPlayers);

	if (IsValid(BoundaryActor))
	{
		BoundaryActor->SetBoundaryMode(EFaerinBoundaryMode::Intro);
	}

	if (bPrimeIntroStartPoseBeforeIntro)
	{
		MulticastPrimeIntroStartPose();
	}

	if (bApplyIntroStartLoopBeforeIntro)
	{
		MulticastApplyIntroStartLoopAnimations();
	}

	SetEncounterState(EFaerinEncounterState::IntroCutsceneDialogue);
	MulticastPlayEncounterSequence(EFaerinEncounterSequence::Intro);

	const float Duration = GetSequenceDurationSeconds(IntroSequence, IntroSequenceFallbackSeconds);
	if (Duration <= UE_SMALL_NUMBER)
	{
		FinishIntroSequence();
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			IntroSequenceTimerHandle,
			this,
			&ThisClass::FinishIntroSequence,
			Duration,
			false);
	}
}

void APRFaerinEncounterDirector::FinishIntroSequence()
{
	if (!HasAuthority() || CurrentState != EFaerinEncounterState::IntroCutsceneDialogue)
	{
		return;
	}

	if (IsValid(BoundaryActor))
	{
		BoundaryActor->SetBoundaryMode(EFaerinBoundaryMode::Negotiation);
	}

	MulticastSetPresentationActorsHidden(false);
	MulticastApplyNegotiationPresentationIdle();
	SetEncounterState(EFaerinEncounterState::NegotiationIdle);
}

void APRFaerinEncounterDirector::StartFightSequence()
{
	SetEncounterState(EFaerinEncounterState::PreFightCutscene);
	MulticastPlayEncounterSequence(EFaerinEncounterSequence::FightStart);

	const float Duration = GetSequenceDurationSeconds(FightStartSequence, FightStartSequenceFallbackSeconds);
	if (Duration <= UE_SMALL_NUMBER)
	{
		FinishFightSequence();
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			FightStartSequenceTimerHandle,
			this,
			&ThisClass::FinishFightSequence,
			Duration,
			false);
	}
}

void APRFaerinEncounterDirector::FinishFightSequence()
{
	if (!HasAuthority() || CurrentState != EFaerinEncounterState::PreFightCutscene)
	{
		return;
	}

	SetEncounterState(EFaerinEncounterState::CombatStarting);
	MulticastSetPresentationActorsHidden(true);

	SpawnedCombatBoss = SpawnCombatBossFromSpawner();
	if (!IsValid(SpawnedCombatBoss))
	{
		UE_LOG(LogTemp, Error, TEXT("FaerinEncounterDirector failed to spawn combat boss."));
		RecoverFightStartFailure();
		return;
	}

	BindCombatBossDefeat(SpawnedCombatBoss);
	SeedCombatBossThreat();

	if (APRBossBaseCharacter* BossCharacter = Cast<APRBossBaseCharacter>(SpawnedCombatBoss))
	{
		BossCharacter->RequestBossEncounterBegin();
	}

	if (IsValid(BoundaryActor))
	{
		BoundaryActor->SetBoundaryMode(EFaerinBoundaryMode::Combat);
	}

	TArray<APRPlayerCharacter*> Participants;
	GetPlayersFromSet(CombatStartParticipants, Participants);
	SetInputLockedForPlayers(Participants, false);
	SetEncounterState(EFaerinEncounterState::Combat);
}

void APRFaerinEncounterDirector::ResetCombatBoss()
{
	bResettingCombatBoss = true;
	UnbindCombatBossDefeat();

	if (IsValid(BossSpawnerActor) && BossSpawnerActor->GetClass()->ImplementsInterface(UPRBossSpawnProviderInterface::StaticClass()))
	{
		IPRBossSpawnProviderInterface::Execute_ResetBossForEncounter(BossSpawnerActor, SpawnedCombatBoss);
	}
	else if (IsValid(SpawnedCombatBoss))
	{
		SpawnedCombatBoss->Destroy();
	}

	SpawnedCombatBoss = nullptr;
	bResettingCombatBoss = false;
}

void APRFaerinEncounterDirector::RestoreNegotiationAfterWipe()
{
	ResetCombatBoss();
	CloseChoiceUIForInstigator();
	MulticastSetPresentationActorsHidden(false);
	MulticastApplyNegotiationPresentationIdle();

	TArray<APRPlayerCharacter*> Participants;
	GetPlayersFromSet(CombatStartParticipants, Participants);
	SetInputLockedForPlayers(Participants, false);

	if (IsValid(BoundaryActor))
	{
		BoundaryActor->ClearEncounterState();
		BoundaryActor->SetBoundaryMode(EFaerinBoundaryMode::Negotiation);
	}

	CombatStartParticipants.Reset();
	ChoiceInstigator = nullptr;
	SetEncounterState(EFaerinEncounterState::NegotiationIdle);
}

void APRFaerinEncounterDirector::RecoverFightStartFailure()
{
	TArray<APRPlayerCharacter*> Participants;
	GetPlayersFromSet(CombatStartParticipants, Participants);
	CloseChoiceUIForInstigator();
	SetInputLockedForPlayers(Participants, false);
	MulticastSetPresentationActorsHidden(false);
	MulticastApplyNegotiationPresentationIdle();

	if (IsValid(BoundaryActor))
	{
		BoundaryActor->SetBoundaryMode(EFaerinBoundaryMode::Negotiation);
	}

	SpawnedCombatBoss = nullptr;
	bResettingCombatBoss = false;
	SetEncounterState(EFaerinEncounterState::NegotiationIdle);
}

void APRFaerinEncounterDirector::SnapshotCombatStartParticipants(APRPlayerCharacter* FallbackPlayer)
{
	CombatStartParticipants.Reset();

	TArray<APRPlayerCharacter*> InsidePlayers;
	if (IsValid(BoundaryActor))
	{
		BoundaryActor->GetArenaInsidePlayers(InsidePlayers);
	}

	for (APRPlayerCharacter* Player : InsidePlayers)
	{
		if (IsEligibleEncounterPlayer(Player))
		{
			AddPlayer(CombatStartParticipants, Player);
		}
	}

	if (CombatStartParticipants.Num() == 0 && IsEligibleEncounterPlayer(FallbackPlayer))
	{
		AddPlayer(CombatStartParticipants, FallbackPlayer);
	}
}

// ===== 플레이어 배치/입력 잠금 =====

void APRFaerinEncounterDirector::SetInputLockedForPlayers(const TArray<APRPlayerCharacter*>& Players, bool bLock) const
{
	for (APRPlayerCharacter* Player : Players)
	{
		if (!IsValid(Player))
		{
			continue;
		}

		APRPlayerController* PlayerController = Cast<APRPlayerController>(Player->GetController());
		if (IsValid(PlayerController))
		{
			PlayerController->ClientSetEncounterInputLock(bLock);
		}
	}
}

void APRFaerinEncounterDirector::AlignCombatStartParticipants()
{
	TArray<APRPlayerCharacter*> Participants;
	GetPlayersFromSet(CombatStartParticipants, Participants);
	if (Participants.Num() == 0)
	{
		return;
	}

	Participants.RemoveAll([](APRPlayerCharacter* Player)
	{
		return !IsValid(Player);
	});

	APRPlayerCharacter* CenterPlayer = IsValid(ChoiceInstigator) && Participants.Contains(ChoiceInstigator.Get())
		? ChoiceInstigator.Get()
		: Participants[0];
	Participants.Remove(CenterPlayer);

	ApplySlotTransform(CenterPlayer, ResolveSlotTransform(SlotCenterActor, PlayerSlotCenter));

	if (Participants.Num() >= 1)
	{
		ApplySlotTransform(Participants[0], ResolveSlotTransform(SlotLeftActor, PlayerSlotLeft));
	}

	if (Participants.Num() >= 2)
	{
		ApplySlotTransform(Participants[1], ResolveSlotTransform(SlotRightActor, PlayerSlotRight));
	}
}

void APRFaerinEncounterDirector::ApplySlotTransform(APRPlayerCharacter* Player, const FTransform& SlotTransform) const
{
	if (!IsValid(Player))
	{
		return;
	}

	if (UCharacterMovementComponent* MovementComponent = Player->GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}

	Player->SetActorTransform(SlotTransform, false, nullptr, ETeleportType::TeleportPhysics);

	if (UCharacterMovementComponent* MovementComponent = Player->GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}
}

FTransform APRFaerinEncounterDirector::ResolveSlotTransform(AActor* SlotActor, const FTransform& FallbackTransform) const
{
	return IsValid(SlotActor) ? SlotActor->GetActorTransform() : FallbackTransform;
}

// ===== 보스 스폰/위협 주입 =====

AActor* APRFaerinEncounterDirector::SpawnCombatBossFromSpawner()
{
	if (!IsValid(BossSpawnerActor) || !BossSpawnerActor->GetClass()->ImplementsInterface(UPRBossSpawnProviderInterface::StaticClass()))
	{
		return nullptr;
	}

	return IPRBossSpawnProviderInterface::Execute_SpawnBossForEncounter(BossSpawnerActor);
}

void APRFaerinEncounterDirector::BindCombatBossDefeat(AActor* Boss)
{
	if (!IsValid(Boss))
	{
		return;
	}

	Boss->OnDestroyed.RemoveDynamic(this, &ThisClass::HandleSpawnedCombatBossDestroyed);
	Boss->OnDestroyed.AddDynamic(this, &ThisClass::HandleSpawnedCombatBossDestroyed);
	bCombatBossDefeated = false;
}

void APRFaerinEncounterDirector::UnbindCombatBossDefeat()
{
	AActor* Boss = SpawnedCombatBoss.Get();
	if (Boss == nullptr)
	{
		return;
	}

	Boss->OnDestroyed.RemoveDynamic(this, &ThisClass::HandleSpawnedCombatBossDestroyed);
}

void APRFaerinEncounterDirector::SeedCombatBossThreat()
{
	TArray<APRPlayerCharacter*> Participants;
	GetPlayersFromSet(CombatStartParticipants, Participants);
	AddThreatToCombatBoss(Participants, ChoiceInstigator, InitialEncounterThreat);
}

void APRFaerinEncounterDirector::AddThreatToCombatBoss(
	const TArray<APRPlayerCharacter*>& Participants,
	APRPlayerCharacter* PrimaryTarget,
	float ThreatAmount)
{
	if (!HasAuthority() || !IsValid(SpawnedCombatBoss))
	{
		return;
	}

	if (APRFaerinCharacter* FaerinCharacter = Cast<APRFaerinCharacter>(SpawnedCombatBoss))
	{
		FaerinCharacter->SeedEncounterTargets(Participants, PrimaryTarget, ThreatAmount);
		return;
	}

	const IPREnemyInterface* EnemyInterface = Cast<IPREnemyInterface>(SpawnedCombatBoss);
	UPREnemyThreatComponent* ThreatComponent = EnemyInterface != nullptr
		? EnemyInterface->GetEnemyThreatComponent()
		: nullptr;
	if (!IsValid(ThreatComponent))
	{
		return;
	}

	APRPlayerCharacter* ResolvedPrimaryTarget = IsValid(PrimaryTarget) ? PrimaryTarget : nullptr;
	const float ResolvedThreatAmount = FMath::Max(ThreatAmount, 0.0f);
	for (APRPlayerCharacter* Participant : Participants)
	{
		if (!IsValid(Participant))
		{
			continue;
		}

		if (ResolvedThreatAmount > UE_SMALL_NUMBER)
		{
			ThreatComponent->AddThreat(Participant, ResolvedThreatAmount);
		}

		if (!IsValid(ResolvedPrimaryTarget))
		{
			ResolvedPrimaryTarget = Participant;
		}
	}

	if (IsValid(PrimaryTarget) && !Participants.Contains(PrimaryTarget) && ResolvedThreatAmount > UE_SMALL_NUMBER)
	{
		ThreatComponent->AddThreat(PrimaryTarget, ResolvedThreatAmount);
	}

	if (IsValid(ResolvedPrimaryTarget))
	{
		ThreatComponent->ForceCurrentTarget(ResolvedPrimaryTarget);
	}
}

// ===== 시퀀스 재생 =====

float APRFaerinEncounterDirector::GetSequenceDurationSeconds(ULevelSequence* Sequence, float FallbackSeconds) const
{
	if (!IsValid(Sequence))
	{
		return FMath::Max(FallbackSeconds, 0.0f);
	}

	UMovieScene* MovieScene = Sequence->GetMovieScene();
	if (!IsValid(MovieScene))
	{
		return FMath::Max(FallbackSeconds, 0.0f);
	}

	const TRange<FFrameNumber> PlaybackRange = MovieScene->GetPlaybackRange();
	if (!PlaybackRange.HasLowerBound() || !PlaybackRange.HasUpperBound())
	{
		return FMath::Max(FallbackSeconds, 0.0f);
	}

	const FFrameRate TickResolution = MovieScene->GetTickResolution();
	const double TickResolutionDecimal = TickResolution.AsDecimal();
	if (TickResolutionDecimal <= UE_SMALL_NUMBER)
	{
		return FMath::Max(FallbackSeconds, 0.0f);
	}

	const int32 FrameCount = PlaybackRange.GetUpperBoundValue().Value - PlaybackRange.GetLowerBoundValue().Value;
	return FMath::Max(static_cast<float>(static_cast<double>(FrameCount) / TickResolutionDecimal), 0.0f);
}

void APRFaerinEncounterDirector::ApplyNegotiationPresentationIdleLocal()
{
	if (bUseIntroTailLoopAfterIntro && IntroTailLoopBindings.Num() > 0)
	{
		ApplyIntroTailAnimationLoopsLocal();
		return;
	}

	ApplyConfiguredNegotiationIdleLocal();
}

void APRFaerinEncounterDirector::ApplyConfiguredNegotiationIdleLocal()
{
	if (!bApplyNegotiationIdleAfterIntro)
	{
		return;
	}

	for (const FPRFaerinPresentationIdleBinding& Binding : NegotiationIdleBindings)
	{
		PlayIdleAnimationOnPresentationActor(Binding.Actor, Binding.IdleAnimation, Binding.bLoop);
	}
}

void APRFaerinEncounterDirector::ApplyIntroStartLoopAnimationsLocal()
{
	for (const FPRFaerinPresentationTailLoopBinding& Binding : IntroStartLoopBindings)
	{
		PlaySingleNodeAnimationOnPresentationActor(
			Binding.Actor,
			Binding.LoopAnimation,
			Binding.StartPositionSeconds,
			Binding.bLoop,
			Binding.bPreserveActorTransform);
	}
}

void APRFaerinEncounterDirector::ApplyIntroTailAnimationLoopsLocal()
{
	for (const FPRFaerinPresentationTailLoopBinding& Binding : IntroTailLoopBindings)
	{
		PlaySingleNodeAnimationOnPresentationActor(
			Binding.Actor,
			Binding.LoopAnimation,
			Binding.StartPositionSeconds,
			Binding.bLoop,
			Binding.bPreserveActorTransform);
	}
}

void APRFaerinEncounterDirector::PrimeIntroStartPoseLocal()
{
	if (!IsValid(IntroSequence))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	UMovieScene* MovieScene = IntroSequence->GetMovieScene();
	if (!IsValid(MovieScene))
	{
		return;
	}

	const TRange<FFrameNumber> PlaybackRange = MovieScene->GetPlaybackRange();
	if (!PlaybackRange.HasLowerBound())
	{
		return;
	}

	FMovieSceneSequencePlaybackSettings PlaybackSettings;
	PlaybackSettings.bDisableCameraCuts = true;
	PlaybackSettings.FinishCompletionStateOverride = EMovieSceneCompletionModeOverride::ForceKeepState;

	ALevelSequenceActor* SequenceActor = nullptr;
	ULevelSequencePlayer* SequencePlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(
		World,
		IntroSequence,
		PlaybackSettings,
		SequenceActor);
	if (!IsValid(SequencePlayer))
	{
		return;
	}

	const FFrameTime StartFrameTime(PlaybackRange.GetLowerBoundValue());
	SequencePlayer->SetPlaybackPosition(FMovieSceneSequencePlaybackParams(StartFrameTime, EUpdatePositionMethod::Jump));

	if (IsValid(SequenceActor))
	{
		SequenceActor->Destroy();
	}
}

void APRFaerinEncounterDirector::PlayIdleAnimationOnPresentationActor(
	AActor* Actor,
	UAnimSequenceBase* Animation,
	bool bLoop) const
{
	PlaySingleNodeAnimationOnPresentationActor(Actor, Animation, 0.0f, bLoop, true);
}

void APRFaerinEncounterDirector::PlaySingleNodeAnimationOnPresentationActor(
	AActor* Actor,
	UAnimationAsset* Animation,
	float StartPositionSeconds,
	bool bLoop,
	bool bPreserveActorTransform) const
{
	if (!IsValid(Actor) || !IsValid(Animation))
	{
		return;
	}

	USkeletalMeshComponent* MeshComponent = Actor->FindComponentByClass<USkeletalMeshComponent>();
	if (!IsValid(MeshComponent))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("FaerinEncounterDirector presentation actor has no SkeletalMeshComponent. Actor=%s"),
			*Actor->GetName());
		return;
	}

	const FTransform SavedActorTransform = Actor->GetActorTransform();
	const FTransform SavedMeshRelativeTransform = MeshComponent->GetRelativeTransform();
	const float ClampedStartPosition = FMath::Max(StartPositionSeconds, 0.0f);

	MeshComponent->PlayAnimation(Animation, bLoop);
	if (UAnimSingleNodeInstance* SingleNodeInstance = Cast<UAnimSingleNodeInstance>(MeshComponent->GetAnimInstance()))
	{
		SingleNodeInstance->SetLooping(bLoop);
		SingleNodeInstance->SetPosition(ClampedStartPosition, false);
		SingleNodeInstance->SetPlaying(true);
	}

	if (bPreserveActorTransform)
	{
		Actor->SetActorTransform(SavedActorTransform, false, nullptr, ETeleportType::TeleportPhysics);
		MeshComponent->SetRelativeTransform(SavedMeshRelativeTransform);
	}
}

void APRFaerinEncounterDirector::PlaySequenceLocal(EFaerinEncounterSequence SequenceType)
{
	ULevelSequence* Sequence = SequenceType == EFaerinEncounterSequence::Intro
		? IntroSequence.Get()
		: FightStartSequence.Get();
	if (!IsValid(Sequence))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	FMovieSceneSequencePlaybackSettings PlaybackSettings;
	if (SequenceType == EFaerinEncounterSequence::Intro)
	{
		PlaybackSettings.FinishCompletionStateOverride = EMovieSceneCompletionModeOverride::ForceKeepState;
	}

	ALevelSequenceActor* SequenceActor = nullptr;
	ULevelSequencePlayer* SequencePlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(
		World,
		Sequence,
		PlaybackSettings,
		SequenceActor);
	if (!IsValid(SequencePlayer))
	{
		return;
	}

	if (IsValid(SequenceActor))
	{
		ActiveSequenceActors.Add(SequenceActor);
	}

	ActiveSequencePlayers.Add(SequencePlayer);
	SequencePlayer->Play();
}

void APRFaerinEncounterDirector::StopLocalSequences()
{
	for (ULevelSequencePlayer* SequencePlayer : ActiveSequencePlayers)
	{
		if (IsValid(SequencePlayer))
		{
			SequencePlayer->Stop();
		}
	}
	ActiveSequencePlayers.Reset();

	for (ALevelSequenceActor* SequenceActor : ActiveSequenceActors)
	{
		if (IsValid(SequenceActor))
		{
			SequenceActor->Destroy();
		}
	}
	ActiveSequenceActors.Reset();
}

void APRFaerinEncounterDirector::OpenChoiceUIForPlayer(APRPlayerCharacter* Player)
{
	if (!IsValid(Player))
	{
		return;
	}

	APRPlayerController* PlayerController = Cast<APRPlayerController>(Player->GetController());
	if (!IsValid(PlayerController))
	{
		return;
	}

	PlayerController->ClientOpenFaerinEncounterChoice(this);
}

void APRFaerinEncounterDirector::CloseChoiceUIForInstigator()
{
	APRPlayerCharacter* Player = ChoiceInstigator.Get();
	if (!IsValid(Player))
	{
		return;
	}

	APRPlayerController* PlayerController = Cast<APRPlayerController>(Player->GetController());
	if (!IsValid(PlayerController))
	{
		return;
	}

	PlayerController->ClientCloseFaerinEncounterChoice();
}

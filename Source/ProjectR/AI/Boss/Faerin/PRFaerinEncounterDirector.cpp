// Copyright ProjectR. All Rights Reserved.

#include "PRFaerinEncounterDirector.h"

#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/AudioComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
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
#include "Sound/SoundBase.h"

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
	ClearDialogueEmotePlaybackLocal(false);
	StopDialogueVoiceLocal();
	StopSequenceVoiceCuesLocal(TEXT("EndPlay"));

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
		UE_LOG(LogTemp, Log, TEXT("[FaerinDialogueCamera] StageShotId=%s"), *Node.CameraShotId.ToString());
		ReceiveDialogueStageShotChanged(Node.CameraShotId);
	}

	if (Node.NodeType == EPRFaerinDialogueNodeType::Dialog)
	{
		HandleDialogueVoiceEventLocal(Node.VoiceEventPath, Node.NodeId);
		if (Node.VoiceEventPath.IsValid())
		{
			ReceiveDialogueVoiceEvent(Node.VoiceEventPath);
		}

		if (!Node.EmoteId.IsNone() || !Node.EmoteObjectPath.IsEmpty())
		{
			HandleDialogueEmoteChangedLocal(Node.EmoteId, Node.EmoteObjectPath);
			ReceiveDialogueEmoteChanged(Node.EmoteId, Node.EmoteObjectPath);
		}
	}
	else
	{
		StopDialogueVoiceLocal();
	}

	ReceiveDialogueNodePresented(Node);
}

const FPRFaerinDialogueStageShotCameraBinding* APRFaerinEncounterDirector::FindDialogueStageShotCameraBinding(
	FName CameraShotId) const
{
	if (CameraShotId.IsNone())
	{
		return nullptr;
	}

	for (const FPRFaerinDialogueStageShotCameraBinding& Binding : DialogueStageShotCameraBindings)
	{
		if (Binding.CameraShotId == CameraShotId)
		{
			return &Binding;
		}
	}

	return nullptr;
}

void APRFaerinEncounterDirector::ClearDialogueEmotePlaybackLocal(bool bRestoreIdle)
{
	ClearDialogueEmoteReturnTimerLocal();

	if (bRestoreIdle && bDialogueEmotePlaying)
	{
		RestoreDialogueEmoteIdleLocal();
		return;
	}

	bDialogueEmotePlaying = false;
}

void APRFaerinEncounterDirector::StopDialogueVoiceLocal()
{
	if (!IsValid(ActiveDialogueVoiceAudioComponent))
	{
		ActiveDialogueVoiceAudioComponent = nullptr;
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[FaerinDialogueVoice] Stop active dialogue voice"));
	ActiveDialogueVoiceAudioComponent->Stop();
	ActiveDialogueVoiceAudioComponent = nullptr;
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
	if (bPresentationHidden)
	{
		ClearDialogueEmotePlaybackLocal(false);
		StopDialogueVoiceLocal();
		StopSequenceVoiceCuesLocal(TEXT("PresentationHidden"));
	}

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

void APRFaerinEncounterDirector::MulticastStopSequenceVoiceCues_Implementation(FName Reason)
{
	StopSequenceVoiceCuesLocal(Reason);
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

	SetInputLockedForPlayers(IntroPlayers, true);

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

	MulticastStopSequenceVoiceCues(TEXT("IntroFinished"));

	if (IsValid(BoundaryActor))
	{
		BoundaryActor->SetBoundaryMode(EFaerinBoundaryMode::Negotiation);
	}

	MulticastSetPresentationActorsHidden(false);
	MulticastApplyNegotiationPresentationIdle();

	TArray<APRPlayerCharacter*> IntroPlayers;
	GetPlayersFromSet(IntroEnteredPlayers, IntroPlayers);
	RestoreViewTargetForPlayers(IntroPlayers, IntroCameraRestoreBlendTime, TEXT("IntroFinished"));
	SetInputLockedForPlayers(IntroPlayers, false);
	SetEncounterState(EFaerinEncounterState::NegotiationIdle);
}

void APRFaerinEncounterDirector::StartFightSequence()
{
	ClearDialogueEmotePlaybackLocal(false);
	StopDialogueVoiceLocal();
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

	MulticastStopSequenceVoiceCues(TEXT("FightStartFinished"));
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
	RestoreViewTargetForPlayers(Participants, FightStartCameraRestoreBlendTime, TEXT("FightStartFinished"));
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
	MulticastStopSequenceVoiceCues(TEXT("FightStartFailure"));
	CloseChoiceUIForInstigator();
	RestoreViewTargetForPlayers(Participants, FightStartCameraRestoreBlendTime, TEXT("FightStartFailure"));
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

void APRFaerinEncounterDirector::RestoreViewTargetForPlayers(
	const TArray<APRPlayerCharacter*>& Players,
	float BlendTime,
	const TCHAR* Reason) const
{
	const float ClampedBlendTime = FMath::Max(BlendTime, 0.0f);
	for (APRPlayerCharacter* Player : Players)
	{
		if (!IsValid(Player))
		{
			continue;
		}

		APRPlayerController* PlayerController = Cast<APRPlayerController>(Player->GetController());
		if (!IsValid(PlayerController))
		{
			continue;
		}

		UE_LOG(
			LogTemp,
			Log,
			TEXT("[FaerinEncounterCamera] RestoreViewTarget player=%s blend=%.2f reason=%s"),
			*Player->GetName(),
			ClampedBlendTime,
			Reason != nullptr ? Reason : TEXT("None"));
		PlayerController->ClientRestoreFaerinEncounterViewTarget(ClampedBlendTime);
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

void APRFaerinEncounterDirector::HandleDialogueVoiceEventLocal(const FSoftObjectPath& VoiceEventPath, FName NodeId)
{
	StopDialogueVoiceLocal();

	const FString PathString = VoiceEventPath.ToString().TrimStartAndEnd();
	if (PathString.IsEmpty())
	{
		UE_LOG(LogTemp, Verbose, TEXT("[FaerinDialogueVoice] Empty voice path for node=%s"), *NodeId.ToString());
		return;
	}

	UObject* LoadedObject = VoiceEventPath.TryLoad();
	if (!IsValid(LoadedObject))
	{
		UE_LOG(LogTemp, Warning, TEXT("[FaerinDialogueVoice] Failed to load voice path=%s"), *PathString);
		return;
	}

	USoundBase* Sound = Cast<USoundBase>(LoadedObject);
	if (!IsValid(Sound))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[FaerinDialogueVoice] Loaded non-sound asset path=%s class=%s"),
			*PathString,
			*LoadedObject->GetClass()->GetName());
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	AActor* VoiceActor = ResolveDialogueVoiceActor();
	USceneComponent* AttachComponent = ResolveDialogueVoiceAttachComponent(VoiceActor);
	if (IsValid(AttachComponent))
	{
		ActiveDialogueVoiceAudioComponent = UGameplayStatics::SpawnSoundAttached(
			Sound,
			AttachComponent,
			NAME_None,
			FVector::ZeroVector,
			EAttachLocation::KeepRelativeOffset,
			false,
			1.0f,
			1.0f,
			0.0f);
	}
	else if (IsValid(VoiceActor))
	{
		ActiveDialogueVoiceAudioComponent = UGameplayStatics::SpawnSoundAtLocation(
			World,
			Sound,
			VoiceActor->GetActorLocation());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[FaerinDialogueVoice] DialogueVoiceActor is not set."));
		return;
	}

	if (IsValid(ActiveDialogueVoiceAudioComponent))
	{
		UE_LOG(LogTemp, Log, TEXT("[FaerinDialogueVoice] Play voice path=%s sound=%s"), *PathString, *Sound->GetName());
	}
}

void APRFaerinEncounterDirector::HandleDialogueEmoteChangedLocal(FName EmoteId, const FString& EmoteObjectPath)
{
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[FaerinDialogueEmote] EmoteId=%s Path=%s"),
		*EmoteId.ToString(),
		*EmoteObjectPath);

	ClearDialogueEmoteReturnTimerLocal();

	const FString PathString = EmoteObjectPath.TrimStartAndEnd();
	if (PathString.IsEmpty())
	{
		if (bDialogueEmotePlaying)
		{
			RestoreDialogueEmoteIdleLocal();
		}
		return;
	}

	if (PathString.StartsWith(TEXT("/Game/_Core/Emotes/")))
	{
		UE_LOG(LogTemp, Verbose, TEXT("[FaerinDialogueEmote] Skip generic emote path=%s"), *PathString);
		if (bDialogueEmotePlaying)
		{
			RestoreDialogueEmoteIdleLocal();
		}
		return;
	}

	const FSoftObjectPath EmotePath(PathString);
	if (EmotePath.IsNull())
	{
		if (bDialogueEmotePlaying)
		{
			RestoreDialogueEmoteIdleLocal();
		}
		return;
	}

	UObject* LoadedObject = EmotePath.TryLoad();
	UAnimSequence* AnimSequence = Cast<UAnimSequence>(LoadedObject);
	if (!IsValid(AnimSequence))
	{
		UE_LOG(LogTemp, Warning, TEXT("[FaerinDialogueEmote] AnimSequence load failed. Path=%s"), *PathString);
		if (bDialogueEmotePlaying)
		{
			RestoreDialogueEmoteIdleLocal();
		}
		return;
	}

	AActor* TargetActor = ResolveDialogueEmoteActor();
	if (!IsValid(TargetActor))
	{
		UE_LOG(LogTemp, Warning, TEXT("[FaerinDialogueEmote] DialogueEmoteActor is not set."));
		bDialogueEmotePlaying = false;
		return;
	}

	const float Duration = FMath::Max(0.05f, AnimSequence->GetPlayLength());
	UE_LOG(LogTemp, Log, TEXT("[FaerinDialogueEmote] Play Anim=%s Duration=%.3f"), *AnimSequence->GetName(), Duration);

	PlaySingleNodeAnimationOnPresentationActor(TargetActor, AnimSequence, 0.0f, false, true);
	bDialogueEmotePlaying = true;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			DialogueEmoteReturnTimerHandle,
			this,
			&ThisClass::RestoreDialogueEmoteIdleLocal,
			Duration,
			false);
	}
}

void APRFaerinEncounterDirector::RestoreDialogueEmoteIdleLocal()
{
	ClearDialogueEmoteReturnTimerLocal();
	bDialogueEmotePlaying = false;
	ApplyNegotiationPresentationIdleLocal();
}

void APRFaerinEncounterDirector::ClearDialogueEmoteReturnTimerLocal()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DialogueEmoteReturnTimerHandle);
	}
}

AActor* APRFaerinEncounterDirector::ResolveDialogueEmoteActor() const
{
	if (IsValid(DialogueEmoteActor))
	{
		return DialogueEmoteActor;
	}

	for (AActor* PresentationActor : PresentationActors)
	{
		if (IsValid(PresentationActor) && PresentationActor->GetName().Contains(TEXT("Cine_Faerin")))
		{
			return PresentationActor;
		}
	}

	return nullptr;
}

void APRFaerinEncounterDirector::ForceDetachFightStartFaerinPresentationActorLocal(FName Reason)
{
	AActor* FaerinActor = ResolveDialogueEmoteActor();
	const FString ReasonString = Reason.ToString();
	if (!IsValid(FaerinActor))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[FaerinFightStartDetach] %s: Faerin presentation actor missing."),
			*ReasonString);
		return;
	}

	const FTransform SavedActorWorldTransform = FaerinActor->GetActorTransform();
	USkeletalMeshComponent* FaerinMeshComponent = FaerinActor->FindComponentByClass<USkeletalMeshComponent>();
	const bool bHasFaerinMeshComponent = IsValid(FaerinMeshComponent);
	const FTransform SavedRootWorldTransform = IsValid(FaerinActor->GetRootComponent())
		? FaerinActor->GetRootComponent()->GetComponentTransform()
		: SavedActorWorldTransform;
	const FTransform SavedMeshWorldTransform = bHasFaerinMeshComponent ? FaerinMeshComponent->GetComponentTransform() : FTransform::Identity;
	const FTransform SavedMeshRelativeTransform = bHasFaerinMeshComponent ? FaerinMeshComponent->GetRelativeTransform() : FTransform::Identity;
	AActor* ParentActor = FaerinActor->GetAttachParentActor();
	USceneComponent* FaerinRootComponent = FaerinActor->GetRootComponent();
	USceneComponent* AttachParentComponent = IsValid(FaerinRootComponent) ? FaerinRootComponent->GetAttachParent() : nullptr;
	USceneComponent* MeshAttachParentComponent = bHasFaerinMeshComponent ? FaerinMeshComponent->GetAttachParent() : nullptr;
	const bool bRootAttached = ParentActor != nullptr || AttachParentComponent != nullptr;
	const bool bMeshExternallyAttached = bHasFaerinMeshComponent
		&& MeshAttachParentComponent != nullptr
		&& MeshAttachParentComponent != FaerinRootComponent
		&& MeshAttachParentComponent->GetOwner() != FaerinActor;

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[FaerinFightStartDetach] %s BEFORE: actor=%s parentActor=%s parentComponent=%s meshParent=%s rootAttached=%d meshExternal=%d worldLoc=%s meshWorldLoc=%s"),
		*ReasonString,
		*FaerinActor->GetName(),
		ParentActor != nullptr ? *ParentActor->GetName() : TEXT("None"),
		AttachParentComponent != nullptr ? *AttachParentComponent->GetName() : TEXT("None"),
		MeshAttachParentComponent != nullptr ? *MeshAttachParentComponent->GetName() : TEXT("None"),
		bRootAttached ? 1 : 0,
		bMeshExternallyAttached ? 1 : 0,
		*SavedActorWorldTransform.GetLocation().ToString(),
		bHasFaerinMeshComponent ? *SavedMeshWorldTransform.GetLocation().ToString() : TEXT("None"));

	if (bRootAttached || bMeshExternallyAttached)
	{
		if (bRootAttached)
		{
			FaerinActor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

			if (IsValid(FaerinRootComponent) && FaerinRootComponent->GetAttachParent() != nullptr)
			{
				FaerinRootComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
			}
		}

		if (bMeshExternallyAttached)
		{
			FaerinMeshComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
			const FTransform MeshAnchoredActorWorldTransform = SavedMeshRelativeTransform.Inverse() * SavedMeshWorldTransform;
			FaerinActor->SetActorTransform(MeshAnchoredActorWorldTransform, false, nullptr, ETeleportType::TeleportPhysics);
			FaerinMeshComponent->SetRelativeTransform(SavedMeshRelativeTransform);
		}
		else
		{
			FaerinActor->SetActorTransform(SavedRootWorldTransform, false, nullptr, ETeleportType::TeleportPhysics);
			if (bHasFaerinMeshComponent)
			{
				FaerinMeshComponent->SetRelativeTransform(SavedMeshRelativeTransform);
			}
		}
	}

	if (UCharacterMovementComponent* FaerinMovementComponent = FaerinActor->FindComponentByClass<UCharacterMovementComponent>())
	{
		FaerinMovementComponent->StopMovementImmediately();
	}

	FaerinActor->SetActorHiddenInGame(false);
	if (bHasFaerinMeshComponent)
	{
		FaerinMeshComponent->SetHiddenInGame(false, true);
		FaerinMeshComponent->SetVisibility(true, true);
	}

	ParentActor = FaerinActor->GetAttachParentActor();
	FaerinRootComponent = FaerinActor->GetRootComponent();
	AttachParentComponent = IsValid(FaerinRootComponent) ? FaerinRootComponent->GetAttachParent() : nullptr;
	MeshAttachParentComponent = bHasFaerinMeshComponent ? FaerinMeshComponent->GetAttachParent() : nullptr;

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[FaerinFightStartDetach] %s AFTER: actor=%s parentActor=%s parentComponent=%s meshParent=%s worldLoc=%s meshWorldLoc=%s"),
		*ReasonString,
		*FaerinActor->GetName(),
		ParentActor != nullptr ? *ParentActor->GetName() : TEXT("None"),
		AttachParentComponent != nullptr ? *AttachParentComponent->GetName() : TEXT("None"),
		MeshAttachParentComponent != nullptr ? *MeshAttachParentComponent->GetName() : TEXT("None"),
		*FaerinActor->GetActorLocation().ToString(),
		bHasFaerinMeshComponent ? *FaerinMeshComponent->GetComponentLocation().ToString() : TEXT("None"));
}

AActor* APRFaerinEncounterDirector::ResolveDialogueVoiceActor() const
{
	if (IsValid(DialogueVoiceActor))
	{
		return DialogueVoiceActor;
	}

	return ResolveDialogueEmoteActor();
}

USceneComponent* APRFaerinEncounterDirector::ResolveDialogueVoiceAttachComponent(AActor* VoiceActor) const
{
	if (!IsValid(VoiceActor))
	{
		return nullptr;
	}

	if (USkeletalMeshComponent* MeshComponent = VoiceActor->FindComponentByClass<USkeletalMeshComponent>())
	{
		return MeshComponent;
	}

	return VoiceActor->GetRootComponent();
}

void APRFaerinEncounterDirector::ScheduleSequenceVoiceCuesLocal(EFaerinEncounterSequence SequenceType)
{
	StopSequenceVoiceCuesLocal(TEXT("ScheduleNewSequence"));

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	for (const FPRFaerinSequenceVoiceCue& Cue : SequenceVoiceCues)
	{
		if (Cue.SequenceType != SequenceType || Cue.SoundPath.IsNull())
		{
			continue;
		}

		const float StartTimeSeconds = FMath::Max(Cue.StartTimeSeconds, 0.0f);
		UE_LOG(
			LogTemp,
			Log,
			TEXT("[FaerinSequenceVoice] Schedule sequence=%d cue=%s start=%.3f"),
			static_cast<int32>(SequenceType),
			*Cue.CueId.ToString(),
			StartTimeSeconds);

		if (StartTimeSeconds <= UE_SMALL_NUMBER)
		{
			PlaySequenceVoiceCueLocal(SequenceType, Cue.CueId, Cue.SoundPath, Cue.bAttachToVoiceActor);
			continue;
		}

		FTimerHandle TimerHandle;
		FTimerDelegate TimerDelegate = FTimerDelegate::CreateUObject(
			this,
			&ThisClass::PlaySequenceVoiceCueLocal,
			SequenceType,
			Cue.CueId,
			Cue.SoundPath,
			Cue.bAttachToVoiceActor);
		World->GetTimerManager().SetTimer(TimerHandle, TimerDelegate, StartTimeSeconds, false);
		SequenceVoiceTimerHandles.Add(TimerHandle);
	}
}

void APRFaerinEncounterDirector::StopSequenceVoiceCuesLocal(FName Reason)
{
	if (UWorld* World = GetWorld())
	{
		for (FTimerHandle& TimerHandle : SequenceVoiceTimerHandles)
		{
			World->GetTimerManager().ClearTimer(TimerHandle);
		}
	}
	SequenceVoiceTimerHandles.Reset();

	bool bStoppedAny = false;
	for (UAudioComponent* AudioComponent : ActiveSequenceVoiceAudioComponents)
	{
		if (IsValid(AudioComponent))
		{
			AudioComponent->Stop();
			bStoppedAny = true;
		}
	}
	ActiveSequenceVoiceAudioComponents.Reset();

	if (bStoppedAny)
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("[FaerinSequenceVoice] StopAll reason=%s"),
			*Reason.ToString());
	}
}

void APRFaerinEncounterDirector::PlaySequenceVoiceCueLocal(
	EFaerinEncounterSequence SequenceType,
	FName CueId,
	FSoftObjectPath SoundPath,
	bool bAttachToVoiceActor)
{
	if (SoundPath.IsNull())
	{
		return;
	}

	const FString PathString = SoundPath.ToString();
	UObject* LoadedObject = SoundPath.TryLoad();
	if (!IsValid(LoadedObject))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[FaerinSequenceVoice] Failed to load sequence=%d cue=%s path=%s"),
			static_cast<int32>(SequenceType),
			*CueId.ToString(),
			*PathString);
		return;
	}

	USoundBase* Sound = Cast<USoundBase>(LoadedObject);
	if (!IsValid(Sound))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[FaerinSequenceVoice] Loaded non-sound sequence=%d cue=%s path=%s class=%s"),
			static_cast<int32>(SequenceType),
			*CueId.ToString(),
			*PathString,
			*LoadedObject->GetClass()->GetName());
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	AActor* VoiceActor = ResolveDialogueVoiceActor();
	UAudioComponent* AudioComponent = nullptr;
	if (bAttachToVoiceActor)
	{
		if (USceneComponent* AttachComponent = ResolveDialogueVoiceAttachComponent(VoiceActor))
		{
			AudioComponent = UGameplayStatics::SpawnSoundAttached(
				Sound,
				AttachComponent,
				NAME_None,
				FVector::ZeroVector,
				EAttachLocation::KeepRelativeOffset,
				false,
				1.0f,
				1.0f,
				0.0f);
		}
	}

	if (!IsValid(AudioComponent) && IsValid(VoiceActor))
	{
		AudioComponent = UGameplayStatics::SpawnSoundAtLocation(World, Sound, VoiceActor->GetActorLocation());
	}

	if (!IsValid(AudioComponent))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[FaerinSequenceVoice] Voice actor is not set. sequence=%d cue=%s"),
			static_cast<int32>(SequenceType),
			*CueId.ToString());
		return;
	}

	ActiveSequenceVoiceAudioComponents.Add(AudioComponent);
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[FaerinSequenceVoice] Play sequence=%d cue=%s sound=%s"),
		static_cast<int32>(SequenceType),
		*CueId.ToString(),
		*Sound->GetName());
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
	if (SequenceType == EFaerinEncounterSequence::FightStart)
	{
		ClearDialogueEmotePlaybackLocal(false);
		StopDialogueVoiceLocal();
	}

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

	if (SequenceType == EFaerinEncounterSequence::FightStart)
	{
		const float DetachDelaySeconds = FMath::Max(FightStartFaerinDetachDelaySeconds, 0.0f);
		if (DetachDelaySeconds <= UE_SMALL_NUMBER)
		{
			ForceDetachFightStartFaerinPresentationActorLocal(TEXT("AfterFightStartInitialEvaluation"));
		}
		else if (UWorld* LocalWorld = GetWorld())
		{
			FTimerHandle DetachTimerHandle;
			LocalWorld->GetTimerManager().SetTimer(
				DetachTimerHandle,
				FTimerDelegate::CreateUObject(
					this,
					&ThisClass::ForceDetachFightStartFaerinPresentationActorLocal,
					FName(TEXT("AfterFightStartInitialEvaluation"))),
				DetachDelaySeconds,
				false);
		}
	}

	ScheduleSequenceVoiceCuesLocal(SequenceType);
}

void APRFaerinEncounterDirector::StopLocalSequences()
{
	StopSequenceVoiceCuesLocal(TEXT("StopLocalSequences"));

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

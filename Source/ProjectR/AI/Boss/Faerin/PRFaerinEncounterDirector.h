// Copyright ProjectR. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "PRFaerinEncounterDirector.generated.h"

class ALevelSequenceActor;
class APRFaerinEncounterBoundaryActor;
class APRPlayerCharacter;
class ULevelSequence;
class ULevelSequencePlayer;
class UPRFaerinEncounterDialogueData;

UENUM(BlueprintType)
enum class EFaerinEncounterState : uint8
{
	PreIntro,
	IntroCutsceneDialogue,
	NegotiationIdle,
	ChoiceDialogue,
	Declined,
	PreFightCutscene,
	CombatStarting,
	Combat,
	WipeReset,
	Defeated
};

UENUM(BlueprintType)
enum class EFaerinEncounterSequence : uint8
{
	Intro,
	FightStart
};

// Faerin 컷신 진입부터 실제 전투 보스 스폰까지의 인카운터 상태를 서버 권위로 관리한다.
UCLASS(Blueprintable)
class PROJECTR_API APRFaerinEncounterDirector : public AActor
{
	GENERATED_BODY()

public:
	APRFaerinEncounterDirector();

	/*~ AActor Interface ~*/
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	// Gather 영역에 들어온 플레이어를 등록하고, 필요한 인원이 모두 모이면 Intro를 시작한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Faerin|Encounter")
	void NotifyPlayerEnteredGather(APRPlayerCharacter* Player);

	// Combat 중 arena에 늦게 들어온 플레이어를 전투 위협 후보와 경계 잠금에 추가한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Faerin|Encounter")
	void NotifyPlayerEnteredCombatArena(APRPlayerCharacter* Player);

	// 현재 플레이어가 전투 선택 대화를 열 수 있는지 확인한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Faerin|Encounter")
	bool CanOpenChoiceDialogue(APRPlayerCharacter* Player) const;

	// 클라이언트 상호작용 힌트 표시용 예측 조건을 확인한다. 서버 최종 검증은 CanOpenChoiceDialogue가 담당한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Faerin|Encounter")
	bool CanShowChoiceDialoguePrompt(APRPlayerCharacter* Player) const;

	// 선택 대화 상태로 진입하고 선택권자를 기록한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Faerin|Encounter")
	void StartChoiceDialogue(APRPlayerCharacter* Player);

	// 선택권자가 전투 시작을 선택했을 때 pre-fight 컷신과 보스 스폰 흐름을 시작한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Faerin|Encounter")
	void ChooseFight(APRPlayerCharacter* Player);

	// 선택권자가 전투를 거절했을 때 협상 대기 상태로 되돌릴 수 있는 상태를 기록한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Faerin|Encounter")
	void ChooseDecline(APRPlayerCharacter* Player);

	// 파티 전멸 확정 시 실제 전투 보스를 정리하고 리셋 상태로 진입한다.
	UFUNCTION()
	void HandlePartyWipeConfirmed();

	// 파티 리스폰 완료 시 Intro 없이 협상 상태로 복구한다.
	UFUNCTION()
	void HandlePartyRespawned();

	// Faerin 처치 완료 상태로 전환하고 경계 잠금을 해제한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Faerin|Encounter")
	void MarkDefeated();

	// 현재 인카운터 상태를 반환한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|AI|Boss|Faerin|Encounter")
	EFaerinEncounterState GetCurrentState() const { return CurrentState; }

protected:
	UFUNCTION()
	void OnRep_CurrentState(EFaerinEncounterState PreviousState);

	UFUNCTION()
	void HandleSpawnedCombatBossDestroyed(AActor* DestroyedActor);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayEncounterSequence(EFaerinEncounterSequence SequenceType);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSetPresentationActorsHidden(bool bPresentationHidden);

	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|AI|Boss|Faerin|Encounter")
	void ReceiveEncounterStateChanged(EFaerinEncounterState PreviousState, EFaerinEncounterState NewState);

private:
	bool IsEligibleEncounterPlayer(APRPlayerCharacter* Player) const;
	bool ContainsPlayer(const TSet<TWeakObjectPtr<APRPlayerCharacter>>& Players, APRPlayerCharacter* Player) const;
	void AddPlayer(TSet<TWeakObjectPtr<APRPlayerCharacter>>& Players, APRPlayerCharacter* Player);
	void CleanupPlayerSet(TSet<TWeakObjectPtr<APRPlayerCharacter>>& Players) const;
	void SetEncounterState(EFaerinEncounterState NewState);
	void BuildIntroRequiredPlayers();
	bool AreAllIntroRequiredPlayersEntered() const;
	void StartIntroSequence();
	void FinishIntroSequence();
	void StartFightSequence();
	void FinishFightSequence();
	void ResetCombatBoss();
	void RestoreNegotiationAfterWipe();
	void SnapshotCombatStartParticipants(APRPlayerCharacter* FallbackPlayer);
	void RecoverFightStartFailure();
	void GetPlayersFromSet(const TSet<TWeakObjectPtr<APRPlayerCharacter>>& Players, TArray<APRPlayerCharacter*>& OutPlayers) const;
	void SetInputLockedForPlayers(const TArray<APRPlayerCharacter*>& Players, bool bLock) const;
	void AlignCombatStartParticipants();
	void ApplySlotTransform(APRPlayerCharacter* Player, const FTransform& SlotTransform) const;
	FTransform ResolveSlotTransform(AActor* SlotActor, const FTransform& FallbackTransform) const;
	AActor* SpawnCombatBossFromSpawner();
	void BindCombatBossDefeat(AActor* Boss);
	void UnbindCombatBossDefeat();
	void SeedCombatBossThreat();
	void AddThreatToCombatBoss(const TArray<APRPlayerCharacter*>& Participants, APRPlayerCharacter* PrimaryTarget, float ThreatAmount);
	float GetSequenceDurationSeconds(ULevelSequence* Sequence, float FallbackSeconds) const;
	void PlaySequenceLocal(EFaerinEncounterSequence SequenceType);
	void StopLocalSequences();
	void OpenChoiceUIForPlayer(APRPlayerCharacter* Player);
	void CloseChoiceUIForInstigator();

public:
	// 실제 전투 보스를 생성하는 스포너 Actor. IPRBossSpawnProviderInterface 구현체여야 한다.
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Encounter")
	TObjectPtr<AActor> BossSpawnerActor = nullptr;

	// Faerin 전용 경계 Actor
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Encounter")
	TObjectPtr<APRFaerinEncounterBoundaryActor> BoundaryActor = nullptr;

	// 컷신 staging root. Python/BP 세팅에서 프레젠테이션 Actor 정렬 기준으로 사용한다.
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Encounter")
	TObjectPtr<AActor> CinematicRootActor = nullptr;

	// 컷신용 Faerin/Throne/RoyalArcher 등 프레젠테이션 Actor 묶음
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Encounter")
	TArray<TObjectPtr<AActor>> PresentationActors;

	// 최초 조우 컷신 LevelSequence
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Encounter|Sequence")
	TObjectPtr<ULevelSequence> IntroSequence = nullptr;

	// 전투 시작 컷신 LevelSequence
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Encounter|Sequence")
	TObjectPtr<ULevelSequence> FightStartSequence = nullptr;

	// 컷신/선택지 대사 DataAsset
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Encounter|Dialogue")
	TObjectPtr<UPRFaerinEncounterDialogueData> DialogueData = nullptr;

	// 선택 플레이어 기준 왼쪽 배치 슬롯 Actor
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Encounter|Slots")
	TObjectPtr<AActor> SlotLeftActor = nullptr;

	// 선택 플레이어 배치 슬롯 Actor
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Encounter|Slots")
	TObjectPtr<AActor> SlotCenterActor = nullptr;

	// 선택 플레이어 기준 오른쪽 배치 슬롯 Actor
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Encounter|Slots")
	TObjectPtr<AActor> SlotRightActor = nullptr;

	// 슬롯 Actor가 비어 있을 때 사용할 왼쪽 fallback transform
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Encounter|Slots")
	FTransform PlayerSlotLeft;

	// 슬롯 Actor가 비어 있을 때 사용할 중앙 fallback transform
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Encounter|Slots")
	FTransform PlayerSlotCenter;

	// 슬롯 Actor가 비어 있을 때 사용할 오른쪽 fallback transform
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Encounter|Slots")
	FTransform PlayerSlotRight;

	// Sequence 길이를 읽지 못할 때 사용할 Intro fallback 시간
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Encounter|Sequence", meta = (ClampMin = "0.0"))
	float IntroSequenceFallbackSeconds = 0.0f;

	// Sequence 길이를 읽지 못할 때 사용할 전투 시작 fallback 시간
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Encounter|Sequence", meta = (ClampMin = "0.0"))
	float FightStartSequenceFallbackSeconds = 0.0f;

	// 전투 시작 시 참가자에게 주입할 초기 위협량
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Encounter|Threat", meta = (ClampMin = "0.0"))
	float InitialEncounterThreat = 1000.0f;

	// 전투 중 늦게 진입한 플레이어에게 사용할 보조 위협량
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Encounter|Threat", meta = (ClampMin = "0.0"))
	float LateJoinThreat = 100.0f;

private:
	UPROPERTY(ReplicatedUsing = OnRep_CurrentState, VisibleInstanceOnly, Category = "ProjectR|AI|Boss|Faerin|Encounter")
	EFaerinEncounterState CurrentState = EFaerinEncounterState::PreIntro;

	UPROPERTY(Transient)
	TObjectPtr<AActor> SpawnedCombatBoss = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<APRPlayerCharacter> ChoiceInstigator = nullptr;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ULevelSequencePlayer>> ActiveSequencePlayers;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ALevelSequenceActor>> ActiveSequenceActors;

	TSet<TWeakObjectPtr<APRPlayerCharacter>> IntroRequiredPlayers;
	TSet<TWeakObjectPtr<APRPlayerCharacter>> IntroEnteredPlayers;
	TSet<TWeakObjectPtr<APRPlayerCharacter>> CombatStartParticipants;

	FTimerHandle IntroSequenceTimerHandle;
	FTimerHandle FightStartSequenceTimerHandle;

	bool bResettingCombatBoss = false;
	bool bCombatBossDefeated = false;
};

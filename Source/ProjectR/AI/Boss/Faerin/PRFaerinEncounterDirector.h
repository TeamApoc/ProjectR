// Copyright ProjectR. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectR/AI/Boss/Faerin/PREncounterDialogueData.h"
#include "TimerManager.h"
#include "PRFaerinEncounterDirector.generated.h"

class ALevelSequenceActor;
class APRFaerinEncounterBoundaryActor;
class APRPlayerCharacter;
class UAnimationAsset;
class UAnimSequenceBase;
class ULevelSequence;
class ULevelSequencePlayer;

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
// 협상 대기 상태에서 계속 보여야 하는 연출 Actor와 idle 애니메이션을 묶는다.
USTRUCT(BlueprintType)
struct PROJECTR_API FPRFaerinPresentationIdleBinding
{
	GENERATED_BODY()

	// idle loop를 적용할 연출용 Actor
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Presentation")
	TObjectPtr<AActor> Actor = nullptr;

	// 해당 Actor의 첫 SkeletalMeshComponent에 재생할 idle 애니메이션
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Presentation")
	TObjectPtr<UAnimSequenceBase> IdleAnimation = nullptr;

	// idle 애니메이션 반복 여부
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Presentation")
	bool bLoop = true;
};

// 원작 StageShot ID와 실제 카메라 Actor를 연결하기 위한 편집용 매핑이다.
USTRUCT(BlueprintType)
struct PROJECTR_API FPRFaerinDialogueCameraShotBinding
{
	GENERATED_BODY()

	// 원작 CameraShotDetails.SequenceShotNameID
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Dialogue|Camera")
	FName CameraShotId = NAME_None;

	// StageShot 전환에 사용할 카메라 Actor
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Dialogue|Camera")
	TObjectPtr<AActor> CameraActor = nullptr;
};

// Intro 종료 뒤 마지막 연출 애니메이션을 그대로 loop하기 위한 Actor별 바인딩이다.
USTRUCT(BlueprintType)
struct PROJECTR_API FPRFaerinPresentationTailLoopBinding
{
	GENERATED_BODY()

	// tail loop를 적용할 연출용 Actor
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Presentation")
	TObjectPtr<AActor> Actor = nullptr;

	// Intro 마지막에 이어서 반복 재생할 애니메이션
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Presentation")
	TObjectPtr<UAnimSequenceBase> LoopAnimation = nullptr;

	// loop 시작 위치. Intro 마지막 pose와 맞는 시간을 에디터에서 지정한다.
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Presentation", meta = (ClampMin = "0.0"))
	float StartPositionSeconds = 0.0f;

	// tail 애니메이션 반복 여부
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Presentation")
	bool bLoop = true;

	// 애니메이션 적용 후 Actor/Mesh transform을 적용 전 값으로 되돌릴지 여부
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Presentation")
	bool bPreserveActorTransform = true;
};

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

	// 현재 인카운터 선택 UI가 사용할 대화 DataAsset을 반환한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|AI|Boss|Faerin|Dialogue")
	UPRFaerinEncounterDialogueData* GetDialogueData() const { return DialogueData; }

	// 로컬 선택 UI가 새 대화 노드를 표시했을 때 연출용 메타데이터를 BP로 전달한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Faerin|Dialogue|Presentation")
	void HandleDialogueNodePresentedLocal(const FPRFaerinDialogueNode& Node);

protected:
	UFUNCTION()
	void OnRep_CurrentState(EFaerinEncounterState PreviousState);

	UFUNCTION()
	void HandleSpawnedCombatBossDestroyed(AActor* DestroyedActor);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayEncounterSequence(EFaerinEncounterSequence SequenceType);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSetPresentationActorsHidden(bool bPresentationHidden);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastApplyNegotiationPresentationIdle();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPrimeIntroStartPose();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastApplyIntroStartLoopAnimations();

	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|AI|Boss|Faerin|Encounter")
	void ReceiveEncounterStateChanged(EFaerinEncounterState PreviousState, EFaerinEncounterState NewState);

	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|AI|Boss|Faerin|Dialogue|Presentation")
	void ReceiveDialogueNodePresented(const FPRFaerinDialogueNode& Node);

	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|AI|Boss|Faerin|Dialogue|Presentation")
	void ReceiveDialogueStageShotChanged(FName CameraShotId);

	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|AI|Boss|Faerin|Dialogue|Presentation")
	void ReceiveDialogueVoiceEvent(const FSoftObjectPath& VoiceEventPath);

	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|AI|Boss|Faerin|Dialogue|Presentation")
	void ReceiveDialogueEmoteChanged(FName EmoteId, const FString& EmoteObjectPath);

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
	void ApplyNegotiationPresentationIdleLocal();
	void ApplyConfiguredNegotiationIdleLocal();
	void ApplyIntroStartLoopAnimationsLocal();
	void ApplyIntroTailAnimationLoopsLocal();
	void PrimeIntroStartPoseLocal();
	void PlayIdleAnimationOnPresentationActor(AActor* Actor, UAnimSequenceBase* Animation, bool bLoop) const;
	void PlaySingleNodeAnimationOnPresentationActor(
		AActor* Actor,
		UAnimationAsset* Animation,
		float StartPositionSeconds,
		bool bLoop,
		bool bPreserveActorTransform) const;
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

	// BeginPlay 시점에 Intro 첫 프레임 pose/transform을 미리 평가할지 여부
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Presentation|Intro")
	bool bPrimeIntroStartPoseOnBeginPlay = true;

	// Intro 재생 직전 첫 프레임 pose/transform을 다시 맞출지 여부
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Presentation|Intro")
	bool bPrimeIntroStartPoseBeforeIntro = true;

	// PreIntro 대기 상태에서 Intro 첫 프레임용 loop 애니메이션을 재생할지 여부
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Presentation|Intro")
	bool bApplyIntroStartLoopBeforeIntro = true;

	// Intro 종료 후 별도 idle 대신 Intro tail 애니메이션을 우선 loop할지 여부
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Presentation|Intro")
	bool bUseIntroTailLoopAfterIntro = true;

	// Intro 첫 프레임 pose와 이어지는 Actor별 PreIntro loop 설정
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Presentation|Intro")
	TArray<FPRFaerinPresentationTailLoopBinding> IntroStartLoopBindings;

	// Intro 마지막 pose와 이어지는 Actor별 tail loop 설정
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Presentation|Intro")
	TArray<FPRFaerinPresentationTailLoopBinding> IntroTailLoopBindings;

	// Intro 종료 후 NegotiationIdle에서 보여줄 연출 Actor별 idle loop 설정
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Presentation|Idle")
	TArray<FPRFaerinPresentationIdleBinding> NegotiationIdleBindings;

	// NegotiationIdle 진입/복구 시 idle loop를 자동 적용할지 여부
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Presentation|Idle")
	bool bApplyNegotiationIdleAfterIntro = true;

	// 원작 StageShot ID와 실제 카메라 Actor 편집용 매핑
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin|Dialogue|Camera")
	TArray<FPRFaerinDialogueCameraShotBinding> DialogueCameraShotBindings;

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

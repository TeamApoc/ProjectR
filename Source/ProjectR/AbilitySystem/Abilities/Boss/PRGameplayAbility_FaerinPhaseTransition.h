// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Abilities/Boss/PRGameplayAbility_BossPhaseTransition.h"
#include "PRGameplayAbility_FaerinPhaseTransition.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAnimMontage;
class UBrainComponent;
class UPRFaerinGodFallComponent;
class UPRFaerinGodFallDataAsset;
class APRFaerinCharacter;

// Faerin 전용 PhaseTransition Ability다. Phase2 God Fall 진입과 Phase3 God Fall 속도 강화를 담당한다.
UCLASS()
class PROJECTR_API UPRGameplayAbility_FaerinPhaseTransition : public UPRGameplayAbility_BossPhaseTransition
{
	GENERATED_BODY()

public:
	UPRGameplayAbility_FaerinPhaseTransition();

	/*~ UGameplayAbility Interface ~*/
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	// God Fall 시전 위치 도착 후 본체 몽타주 시퀀스를 시작한다.
	void HandleGodFallCastStarted();

	// God Fall entry 종료 결과를 받아 실제 Phase2 commit 여부를 결정한다.
	void HandleGodFallEntryFinished(bool bSucceeded);

	// God Fall 중 Faerin 본체 몽타주를 순서대로 재생한다.
	void PlayNextBodyMontage();

	// 현재 본체 몽타주 task를 정리한다.
	void ClearBodyMontageTask();

	// God Fall entry와 본체 몽타주가 모두 끝났을 때 Phase2 전환을 확정한다.
	void TryCommitPhase2GodFallTransition();

	// God Fall 전용 연출 중 BT 이동이 보스 위치를 덮어쓰지 못하도록 AI 로직을 멈춘다.
	void PauseOwnerBrainForGodFall(APRFaerinCharacter* FaerinCharacter);

	// God Fall 전용 연출이 끝나면 멈춘 AI 로직을 재개한다.
	void ResumeOwnerBrainForGodFall();

	// PhaseTransition 실패 또는 취소 시 기존 phase로 되돌리고 transition 상태를 정리한다.
	void RollbackPhaseTransition();

	// 실제 phase 확정 전 BGM에 사용할 목표 phase를 예고한다.
	void BroadcastBGMPhasePreview(EPRBossPhase PreviewPhase);

	// PhaseTransition 이벤트 payload에서 목표 phase를 해석한다.
	EPRBossPhase ResolveBossPhaseFromEvent(const FGameplayEventData* TriggerEventData) const;

	// God Fall 본체 몽타주 재생 완료 시 다음 몽타주로 넘어간다.
	UFUNCTION()
	void HandleBodyMontageCompleted();

	// God Fall 본체 몽타주가 중단되면 phase transition을 취소한다.
	UFUNCTION()
	void HandleBodyMontageInterrupted();

private:
	UPROPERTY(Transient)
	TObjectPtr<UPRFaerinGodFallComponent> ActiveGodFallComponent;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> ActiveBodyMontageTask;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UAnimMontage>> BodyMontageQueue;

	TWeakObjectPtr<UBrainComponent> PausedBrainComponent;
	EPRBossPhase PendingTargetPhase = EPRBossPhase::Phase1;
	int32 ActiveBodyMontageIndex = INDEX_NONE;
	bool bPhaseTransitionCommitted = false;
	bool bGodFallCastStarted = false;
	bool bGodFallEntrySucceeded = false;
	bool bBodyMontageSequenceFinished = false;
	bool bPausedBrainLogicForGodFall = false;
	bool bSuppressBodyMontageCallbacks = false;
	bool bBGMPhasePreviewed = false;
};

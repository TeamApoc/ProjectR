// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/WorldSubsystem.h"
#include "ProjectR/Audio/PRBGMTypes.h"
#include "PRBGMSubsystem.generated.h"

class UAudioComponent;
class UPRAbilitySystemComponent;
class USoundBase;
struct FInstancedStruct;

// 월드 단위 BGM 재생과 상태 전환 관리 Subsystem
UCLASS()
class PROJECTR_API UPRBGMSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/*~ UWorldSubsystem Interface ~*/
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

public:
	// 현재 레벨의 기본 BGM 재생 시작
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Audio|BGM")
	void StartLevelBGM();

	// 현재 BGM을 페이드아웃 후 정지
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Audio|BGM")
	void StopBGM(float FadeOutDuration);

	// 지정 상태의 BGM으로 전환
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Audio|BGM")
	void SetBGMState(EPRBGMState NewState);

	// 현재 레벨 기본 BGM으로 복귀
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Audio|BGM")
	void RestoreDefaultLevelBGM();

protected:
	// 현재 월드의 레벨 식별자 계산
	FName ResolveCurrentLevelId() const;

	// 현재 레벨 Entry 조회 및 캐시
	bool TryCacheCurrentLevelEntry();

	// 트랙 사운드 동기 로드
	USoundBase* LoadTrackSound(const FPRBGMTrack& Track) const;

	// 기존 BGM과 새 BGM 크로스페이드
	void PlayTrack(EPRBGMState NewState, const FPRBGMTrack& Track);

	// 페이드아웃 오디오 컴포넌트 정리 예약
	void FadeOutAndDestroyAudioComponent(UAudioComponent* AudioComponent, float FadeOutDuration);

	// 페이드아웃 목록 정리
	void CleanupFadingAudioComponents();

	// 로컬 플레이어 ASC 태그 이벤트 바인딩
	void BindLocalPlayerCombatTag();

	// 로컬 플레이어 ASC 바인딩 재시도 시작
	void StartLocalPlayerCombatTagBindRetry();

	// 로컬 플레이어 ASC 바인딩 재시도 정리
	void StopLocalPlayerCombatTagBindRetry();

	// 로컬 플레이어 ASC 태그 이벤트 해제
	void UnbindLocalPlayerCombatTag();

	// 로컬 플레이어 ASC 조회
	UPRAbilitySystemComponent* ResolveLocalPlayerAbilitySystem() const;

	// 로컬 플레이어 태그 변경 처리
	void HandleLocalPlayerGameplayTagUpdated(const FGameplayTag& Tag, bool bTagExists);

	// 전투 BGM 복귀 지연 타이머 완료 처리
	void HandleCombatBGMReleaseDelayElapsed();

	// 보스 BGM 이벤트 구독
	void BindBossBGMEvents();

	// 보스 BGM 이벤트 구독 해제
	void UnbindBossBGMEvents();

	// 보스 조우 시작 이벤트 처리
	void HandleBossEncounterBeginEvent(FGameplayTag EventTag, const FInstancedStruct& Payload);

	// 보스 조우 종료 이벤트 처리
	void HandleBossEncounterEndEvent(FGameplayTag EventTag, const FInstancedStruct& Payload);

	// 보스 페이즈 변경 이벤트 처리
	void HandleBossPhaseChangedEvent(FGameplayTag EventTag, const FInstancedStruct& Payload);

	// 보스 페이즈 BGM 상태 여부
	bool IsBossPhaseState(EPRBGMState State) const;

private:
	FName CurrentLevelId = NAME_None;

	FPRLevelBGMEntry CurrentLevelEntry;

	bool bHasCurrentLevelEntry = false;

	EPRBGMState CurrentState = EPRBGMState::None;

	TWeakObjectPtr<USoundBase> CurrentSound;

	UPROPERTY()
	TObjectPtr<UAudioComponent> CurrentAudioComponent = nullptr;

	TArray<TWeakObjectPtr<UAudioComponent>> FadingOutAudioComponents;

	TWeakObjectPtr<UPRAbilitySystemComponent> BoundAbilitySystemComponent;

	FDelegateHandle CombatTagDelegateHandle;

	FDelegateHandle BossEncounterBeginDelegateHandle;

	FDelegateHandle BossEncounterEndDelegateHandle;

	FDelegateHandle BossPhaseChangedDelegateHandle;

	FDelegateHandle BossBGMPhasePreviewDelegateHandle;

	FTimerHandle CombatBGMReleaseTimerHandle;

	FTimerHandle LocalPlayerBindRetryTimerHandle;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Audio|BGM", meta = (ClampMin = "0.0", Units = "s"))
	float CombatBGMReleaseDelay = 0.5f;

	float LocalPlayerBindRetryInterval = 0.5f;
};

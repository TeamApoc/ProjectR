// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Net/UnrealNetwork.h"
#include "ProjectR/AbilitySystem/Data/PRAbilitySet.h"
#include "ProjectR/AI/PREnemyAITypes.h"
#include "ProjectR/Character/Enemy/PREnemyBaseCharacter.h"
#include "ProjectR/System/PREventTypes.h"
#include "PRBossBaseCharacter.generated.h"

class UPRAbilitySet;
class APRBossPatternActor;
class APRBossBaseCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPRPhaseChangedSignature, EPRBossPhase, OldPhase, EPRBossPhase, NewPhase);

/**
 * 보스 조우 이벤트 페이로드.
 * Event.Boss.Encounter.Begin 발송 시 동반된다.
 */
USTRUCT(BlueprintType)
struct PROJECTR_API FPRBossEncounterEventPayload : public FPREventPayload
{
	GENERATED_BODY()

	// 조우 대상 보스 인스턴스
	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<APRBossBaseCharacter> Boss = nullptr;
};

// 보스 몬스터 공통 베이스다.
// 일반 적 베이스 위에 페이즈 상태, 페이즈별 AbilitySet, 체력 비율 기반 전환을 얹는다.
UCLASS(Abstract)
class PROJECTR_API APRBossBaseCharacter : public APREnemyBaseCharacter
{
	GENERATED_BODY()

public:
	APRBossBaseCharacter();

	/*~ AActor Interface ~*/
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/*~ APawn Interface ~*/
	virtual void PossessedBy(AController* NewController) override;

	/*~ APRCharacterBase Interface ~*/
	virtual EPRCharacterRole GetCharacterRole() const override { return EPRCharacterRole::Boss; }

	/*~ IPRCombatInterface ~*/
	virtual void OnPostDamageApplied(const FPRDamageAppliedContext& Context) override;

	EPRBossPhase GetCurrentPhase() const { return CurrentPhase; }

	// 체력 비율이 바뀔 때 호출해 다음 페이즈 조건을 검사한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss")
	void OnHealthRatioChanged(float NewRatio);

	// 페이즈 전환 연출/Ability를 시작하기 위한 진입점이다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss")
	void BeginPhaseTransition(EPRBossPhase NewPhase);

	// 전환 연출이 끝난 뒤 실제 페이즈 값을 확정한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss")
	void CommitPhaseTransition(EPRBossPhase NewPhase);

	// 보스가 생성한 지속형 패턴 Actor를 활성 목록에 등록한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss")
	void RegisterBossPatternActor(APRBossPatternActor* Actor);

	// 지속형 패턴 Actor가 만료/파괴될 때 활성 목록에서 제거한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss")
	void UnregisterBossPatternActor(APRBossPatternActor* Actor);

	// 페이즈 전환/사망/그로기 등 보스 상태 전환 시 살아 있는 패턴 Actor를 모두 취소한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss")
	void CancelAllBossPatternActors();

	// Phase 전환 중에도 유지해야 하는 지속형 패턴 Actor를 제외하고 취소한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss")
	void CancelBossPatternActorsForPhaseTransition();

	// 보스 HUD에 표시할 이름을 반환한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|AI|Boss")
	FText GetBossDisplayName() const;

	// 보스 HUD 페이즈 마커에 사용할 HP 임계값 비율 목록을 반환한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss")
	void GetPhaseThresholdRatios(TArray<float>& OutRatios) const;

public:
	UPROPERTY(BlueprintAssignable, Category = "ProjectR|AI|Boss")
	FPRPhaseChangedSignature OnPhaseChanged;

protected:
	/*~ APRCharacterBase Interface ~*/
	virtual void HandleGameplayTagUpdated(const FGameplayTag& ChangedTag, bool TagExists) override;

	UFUNCTION()
	void OnRep_CurrentPhase(EPRBossPhase OldPhase);

protected:
	// 현재 확정된 보스 페이즈다. 클라이언트 연출 동기화를 위해 복제한다.
	UPROPERTY(ReplicatedUsing = OnRep_CurrentPhase, VisibleInstanceOnly, Category = "ProjectR|AI|Boss")
	EPRBossPhase CurrentPhase = EPRBossPhase::Phase1;

	// 페이즈마다 추가로 부여할 AbilitySet이다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|AI|Boss")
	TMap<EPRBossPhase, TObjectPtr<UPRAbilitySet>> PhaseAbilitySets;

	// 체력 비율이 이 값 이하가 되면 해당 페이즈 전환 후보가 된다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|AI|Boss")
	TMap<EPRBossPhase, float> PhaseThresholdRatios;

	// 현재 페이즈에서 부여한 Ability/GE 핸들이다.
	UPROPERTY()
	FPRAbilitySetHandles CurrentPhaseHandles;

	// 전환 연출 중 목표 페이즈를 보관한다.
	EPRBossPhase PendingPhase = EPRBossPhase::Phase1;

	// Portal/Godfall/Shift처럼 Ability 종료 후에도 남을 수 있는 Helper Actor 목록이다.
	UPROPERTY(Transient)
	TArray<TObjectPtr<APRBossPatternActor>> ActivePatternActors;
};

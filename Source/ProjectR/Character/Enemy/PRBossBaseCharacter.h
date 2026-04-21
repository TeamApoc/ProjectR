// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Net/UnrealNetwork.h"
#include "ProjectR/AbilitySystem/Data/PRAbilitySet.h"
#include "ProjectR/AI/PREnemyAITypes.h"
#include "ProjectR/Character/Enemy/PREnemyBaseCharacter.h"
#include "PRBossBaseCharacter.generated.h"

class UPRAbilitySet;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPRPhaseChangedSignature, EPRFaerinPhase, OldPhase, EPRFaerinPhase, NewPhase);

// 보스 몬스터 공통 베이스다.
// 일반 적 베이스 위에 페이즈 상태, 페이즈별 AbilitySet, 체력 비율 기반 전환을 얹는다.
UCLASS(Abstract)
class PROJECTR_API APRBossBaseCharacter : public APREnemyBaseCharacter
{
	GENERATED_BODY()

public:
	APRBossBaseCharacter();

	virtual void PossessedBy(AController* NewController) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual EPRCharacterRole GetCharacterRole() const override { return EPRCharacterRole::Boss; }

	EPRFaerinPhase GetCurrentPhase() const { return CurrentPhase; }

	// 체력 비율이 바뀔 때 호출해 다음 페이즈 조건을 검사한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss")
	void OnHealthRatioChanged(float NewRatio);

	// 페이즈 전환 연출/Ability를 시작하기 위한 진입점이다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss")
	void BeginPhaseTransition(EPRFaerinPhase NewPhase);

	// 전환 연출이 끝난 뒤 실제 페이즈 값을 확정한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss")
	void CommitPhaseTransition(EPRFaerinPhase NewPhase);

public:
	UPROPERTY(BlueprintAssignable, Category = "ProjectR|AI|Boss")
	FPRPhaseChangedSignature OnPhaseChanged;

protected:
	UFUNCTION()
	void OnRep_CurrentPhase(EPRFaerinPhase OldPhase);

protected:
	// 현재 확정된 보스 페이즈다. 클라이언트 연출 동기화를 위해 복제한다.
	UPROPERTY(ReplicatedUsing = OnRep_CurrentPhase, VisibleInstanceOnly, Category = "ProjectR|AI|Boss")
	EPRFaerinPhase CurrentPhase = EPRFaerinPhase::Opening;

	// 페이즈마다 추가로 부여할 AbilitySet이다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|AI|Boss")
	TMap<EPRFaerinPhase, TObjectPtr<UPRAbilitySet>> PhaseAbilitySets;

	// 체력 비율이 이 값 이하가 되면 해당 페이즈 전환 후보가 된다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|AI|Boss")
	TMap<EPRFaerinPhase, float> PhaseThresholdRatios;

	// 현재 페이즈에서 부여한 Ability/GE 핸들이다.
	UPROPERTY()
	FPRAbilitySetHandles CurrentPhaseHandles;

	// 전환 연출 중 목표 페이즈를 보관한다.
	EPRFaerinPhase PendingPhase = EPRFaerinPhase::Opening;
};

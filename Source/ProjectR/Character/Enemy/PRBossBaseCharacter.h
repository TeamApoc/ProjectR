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

	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss")
	void OnHealthRatioChanged(float NewRatio);

	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss")
	void BeginPhaseTransition(EPRFaerinPhase NewPhase);

	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss")
	void CommitPhaseTransition(EPRFaerinPhase NewPhase);

public:
	UPROPERTY(BlueprintAssignable, Category = "ProjectR|AI|Boss")
	FPRPhaseChangedSignature OnPhaseChanged;

protected:
	UFUNCTION()
	void OnRep_CurrentPhase(EPRFaerinPhase OldPhase);

protected:
	UPROPERTY(ReplicatedUsing = OnRep_CurrentPhase, VisibleInstanceOnly, Category = "ProjectR|AI|Boss")
	EPRFaerinPhase CurrentPhase = EPRFaerinPhase::Opening;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|AI|Boss")
	TMap<EPRFaerinPhase, TObjectPtr<UPRAbilitySet>> PhaseAbilitySets;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|AI|Boss")
	TMap<EPRFaerinPhase, float> PhaseThresholdRatios;

	UPROPERTY()
	FPRAbilitySetHandles CurrentPhaseHandles;

	EPRFaerinPhase PendingPhase = EPRFaerinPhase::Opening;
};

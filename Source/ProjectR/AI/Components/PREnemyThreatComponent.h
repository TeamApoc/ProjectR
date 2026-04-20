// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProjectR/AI/PREnemyAITypes.h"
#include "PREnemyThreatComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPRTargetChangedSignature, AActor*, OldTarget, AActor*, NewTarget);

UCLASS(ClassGroup = (ProjectR), meta = (BlueprintSpawnableComponent))
class PROJECTR_API UPREnemyThreatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPREnemyThreatComponent();

	AActor* GetCurrentTarget() const { return CurrentTarget; }

	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI")
	void AddThreat(AActor* Target, float Amount);

	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI")
	void InvalidateCurrentTarget();

	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI")
	void OnTargetLost(AActor* LostTarget);

public:
	UPROPERTY(BlueprintAssignable, Category = "ProjectR|AI")
	FPRTargetChangedSignature OnTargetChanged;

protected:
	void ReevaluateTarget();
	void SetCurrentTarget(AActor* NewTarget);
	void CleanupInvalidEntries();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|AI")
	float SwitchRatio = 1.3f;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|AI")
	float SwitchCooldown = 3.0f;

	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|AI")
	float ThreatForgetTime = 10.0f;

	UPROPERTY()
	TArray<FPRThreatEntry> ThreatList;

	UPROPERTY()
	TObjectPtr<AActor> CurrentTarget = nullptr;

	float LastSwitchTime = -1000.0f;
};

// Copyright ProjectR. All Rights Reserved.

#include "BTService_PRUpdateEnemyCombatBlackboard.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "ProjectR/AI/PREnemyAIDebug.h"
#include "ProjectR/AI/Data/PREnemyCombatDataAsset.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/Character/Enemy/PREnemyInterface.h"
#include "ProjectR/PRGameplayTags.h"

namespace
{
	bool HasCombatBlackboardKey(const UBlackboardComponent* BlackboardComponent, const FName KeyName)
	{
		return IsValid(BlackboardComponent)
			&& KeyName != NAME_None
			&& BlackboardComponent->GetKeyID(KeyName) != FBlackboard::InvalidKey;
	}

	const UPREnemyCombatDataAsset* ResolveCombatDataAsset(const APawn* ControlledPawn)
	{
		const IPREnemyInterface* EnemyInterface = Cast<IPREnemyInterface>(ControlledPawn);
		if (EnemyInterface == nullptr)
		{
			return nullptr;
		}

		return EnemyInterface->GetCombatDataAsset();
	}

	bool IsEnemyDisabled(const APawn* ControlledPawn)
	{
		const IPREnemyInterface* EnemyInterface = Cast<IPREnemyInterface>(ControlledPawn);
		if (EnemyInterface == nullptr)
		{
			return false;
		}

		const UPRAbilitySystemComponent* AbilitySystemComponent = EnemyInterface->GetEnemyAbilitySystemComponent();
		if (!IsValid(AbilitySystemComponent))
		{
			return false;
		}

		return AbilitySystemComponent->HasMatchingGameplayTag(PRGameplayTags::State_Dead)
			|| AbilitySystemComponent->HasMatchingGameplayTag(PRGameplayTags::State_Groggy);
	}

	EPRTacticalMode GetCurrentTacticalMode(const UBlackboardComponent* BlackboardComponent, const FName TacticalModeKey)
	{
		if (!HasCombatBlackboardKey(BlackboardComponent, TacticalModeKey))
		{
			return EPRTacticalMode::Idle;
		}

		return static_cast<EPRTacticalMode>(BlackboardComponent->GetValueAsEnum(TacticalModeKey));
	}

	bool GetCurrentHasLOS(const UBlackboardComponent* BlackboardComponent, const FName HasLOSKey)
	{
		if (!HasCombatBlackboardKey(BlackboardComponent, HasLOSKey))
		{
			return false;
		}

		return BlackboardComponent->GetValueAsBool(HasLOSKey);
	}

	void UpdateAttackPressureValue(
		UBlackboardComponent* BlackboardComponent,
		const APawn* ControlledPawn,
		const UPREnemyCombatDataAsset* CombatDataAsset,
		const FName AttackPressureKey,
		const FName TacticalModeKey,
		const FName HasLOSKey,
		const bool bHasValidTarget,
		const float DeltaSeconds)
	{
		if (!HasCombatBlackboardKey(BlackboardComponent, AttackPressureKey) || !IsValid(CombatDataAsset))
		{
			return;
		}

		const FPREnemyAttackPressureConfig& PressureConfig = CombatDataAsset->AttackPressureConfig;
		const EPRTacticalMode TacticalMode = GetCurrentTacticalMode(BlackboardComponent, TacticalModeKey);
		const bool bHasLOS = GetCurrentHasLOS(BlackboardComponent, HasLOSKey);
		const bool bShouldResetForMissingTarget = PressureConfig.bResetWhenNoTarget && !bHasValidTarget;
		const bool bShouldResetForReturn = PressureConfig.bResetWhenReturning && TacticalMode == EPRTacticalMode::Return;
		const bool bShouldResetForDisabled = PressureConfig.bResetWhenDisabled && IsEnemyDisabled(ControlledPawn);
		if (bShouldResetForMissingTarget || bShouldResetForReturn || bShouldResetForDisabled)
		{
			const float PreviousPressure = BlackboardComponent->GetValueAsFloat(AttackPressureKey);
			BlackboardComponent->SetValueAsFloat(AttackPressureKey, 0.0f);
			if (PREnemyAIDebug::IsAttackPressureLogEnabled() && PreviousPressure > 0.0f)
			{
				const TCHAR* ResetReason = bShouldResetForMissingTarget
					? TEXT("NoTarget")
					: (bShouldResetForReturn ? TEXT("Return") : TEXT("Disabled"));

				UE_LOG(
					LogPREnemyAI,
					Verbose,
					TEXT("[AttackPressure] Reset Reason=%s Prev=%.2f Pawn=%s"),
					ResetReason,
					PreviousPressure,
					*GetNameSafe(ControlledPawn));
			}
			return;
		}

		if (PressureConfig.bRequireLOSForGain && !bHasLOS)
		{
			return;
		}

		const float GainPerSecond = PressureConfig.GetGainPerSecond(TacticalMode);
		if (GainPerSecond <= 0.0f)
		{
			return;
		}

		const float CurrentAttackPressure = BlackboardComponent->GetValueAsFloat(AttackPressureKey);
		const float UpdatedAttackPressure = FMath::Clamp(
			CurrentAttackPressure + GainPerSecond * DeltaSeconds,
			0.0f,
			PressureConfig.MaxPressure);

		BlackboardComponent->SetValueAsFloat(AttackPressureKey, UpdatedAttackPressure);

		if (PREnemyAIDebug::IsAttackPressureLogEnabled() && !FMath::IsNearlyEqual(CurrentAttackPressure, UpdatedAttackPressure))
		{
			UE_LOG(
				LogPREnemyAI,
				Verbose,
				TEXT("[AttackPressure] Gain Mode=%s Prev=%.2f Next=%.2f GainPerSecond=%.2f Pawn=%s"),
				*StaticEnum<EPRTacticalMode>()->GetNameStringByValue(static_cast<int64>(TacticalMode)),
				CurrentAttackPressure,
				UpdatedAttackPressure,
				GainPerSecond,
				*GetNameSafe(ControlledPawn));
		}
	}

	void UpdateTargetTrackingData(
		UBlackboardComponent* BlackboardComponent,
		const APawn* ControlledPawn,
		AActor* CurrentTarget,
		const FName TargetLocationKey,
		const FName DistanceToTargetKey,
		const FName ChargePathClearKey,
		const ECollisionChannel ChargeTraceChannel)
	{
		if (!IsValid(CurrentTarget))
		{
			BlackboardComponent->SetValueAsFloat(DistanceToTargetKey, 0.0f);
			BlackboardComponent->SetValueAsBool(ChargePathClearKey, false);
			return;
		}

		const FVector OwnerLocation = ControlledPawn->GetActorLocation();
		const FVector TargetLocation = CurrentTarget->GetActorLocation();
		const float DistanceToTarget = FVector::Dist(OwnerLocation, TargetLocation);

		BlackboardComponent->SetValueAsVector(TargetLocationKey, TargetLocation);
		BlackboardComponent->SetValueAsFloat(DistanceToTargetKey, DistanceToTarget);

		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PREnemyChargePath), false, ControlledPawn);
		QueryParams.AddIgnoredActor(ControlledPawn);

		FHitResult HitResult;
		const bool bBlocked = ControlledPawn->GetWorld()->LineTraceSingleByChannel(
			HitResult,
			OwnerLocation,
			TargetLocation,
			ChargeTraceChannel,
			QueryParams);

		const bool bChargePathClear = !bBlocked || HitResult.GetActor() == CurrentTarget;
		BlackboardComponent->SetValueAsBool(ChargePathClearKey, bChargePathClear);
	}
}

// ===== 초기화 =====

UBTService_PRUpdateEnemyCombatBlackboard::UBTService_PRUpdateEnemyCombatBlackboard()
{
	NodeName = TEXT("Update Enemy Combat Blackboard");
	Interval = 0.2f;
	RandomDeviation = 0.05f;
}

// ===== Blackboard 갱신 =====

void UBTService_PRUpdateEnemyCombatBlackboard::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	const AAIController* AIController = OwnerComp.GetAIOwner();
	const APawn* ControlledPawn = IsValid(AIController) ? AIController->GetPawn() : nullptr;
	if (!IsValid(BlackboardComponent) || !IsValid(ControlledPawn))
	{
		return;
	}

	AActor* CurrentTarget = Cast<AActor>(BlackboardComponent->GetValueAsObject(CurrentTargetKey));
	const bool bHasValidTarget = IsValid(CurrentTarget);
	BlackboardComponent->SetValueAsBool(HasValidTargetKey, bHasValidTarget);

	const UPREnemyCombatDataAsset* CombatDataAsset = ResolveCombatDataAsset(ControlledPawn);
	UpdateAttackPressureValue(
		BlackboardComponent,
		ControlledPawn,
		CombatDataAsset,
		AttackPressureKey,
		TacticalModeKey,
		HasLOSKey,
		bHasValidTarget,
		DeltaSeconds);

	UpdateTargetTrackingData(
		BlackboardComponent,
		ControlledPawn,
		CurrentTarget,
		TargetLocationKey,
		DistanceToTargetKey,
		ChargePathClearKey,
		ChargeTraceChannel);
}

// ===== 에디터 표시 =====

FString UBTService_PRUpdateEnemyCombatBlackboard::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s\nTarget Key: %s\nDistance Key: %s\nPressure Key: %s"),
		*Super::GetStaticDescription(),
		*CurrentTargetKey.ToString(),
		*DistanceToTargetKey.ToString(),
		*AttackPressureKey.ToString());
}

// Copyright ProjectR. All Rights Reserved.

#include "PRSoldierArmoredDebugDrawComponent.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "ProjectR/AI/Components/PREnemyThreatComponent.h"
#include "ProjectR/AI/Data/PRPatternDataAsset.h"
#include "ProjectR/AI/Data/PRSoldierArmoredCombatDataAsset.h"
#include "ProjectR/AbilitySystem/Abilities/Enemy/SoldierArmored/PRSoldierArmoredGameplayTags.h"
#include "ProjectR/Character/Enemy/PREnemyInterface.h"

namespace
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	TAutoConsoleVariable<int32> CVarPRSoldierArmoredDebugRanges(
		TEXT("pr.EnemyAI.DebugSoldierArmoredRanges"),
		0,
		TEXT("Soldier_Armored 전투 거리 디버그 링 표시"),
		ECVF_Cheat);
#endif
}

// ===== 초기화 =====

UPRSoldierArmoredDebugDrawComponent::UPRSoldierArmoredDebugDrawComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	SetIsReplicatedByDefault(false);
}

// ===== Tick =====

void UPRSoldierArmoredDebugDrawComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!ShouldDrawDebugRanges())
	{
		return;
	}

	DrawCombatRanges();
}

// ===== 조건 확인 =====

bool UPRSoldierArmoredDebugDrawComponent::ShouldDrawDebugRanges() const
{
#if UE_BUILD_SHIPPING || UE_BUILD_TEST
	return false;
#else
	if (!bEnableRangeDebug)
	{
		return false;
	}

	const UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return false;
	}

	if (World->WorldType != EWorldType::PIE && World->WorldType != EWorldType::Game)
	{
		return false;
	}

	const AActor* OwnerActor = GetOwner();
	if (bDrawOnlyAuthority && (!IsValid(OwnerActor) || !OwnerActor->HasAuthority()))
	{
		return false;
	}

	if (bRequireConsoleVariable && CVarPRSoldierArmoredDebugRanges.GetValueOnGameThread() <= 0)
	{
		return false;
	}

	return true;
#endif
}

// ===== 거리 드로우 =====

void UPRSoldierArmoredDebugDrawComponent::DrawCombatRanges() const
{
	const AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor))
	{
		return;
	}

	const AActor* CurrentTarget = GetCurrentTarget();
	const float DistanceToTarget = GetDistanceToTarget(CurrentTarget);
	const FVector Center = OwnerActor->GetActorLocation() + FVector(0.0f, 0.0f, DrawHeightOffset);

	if (IsValid(CombatDataAsset))
	{
		DrawMaxRange(TEXT("Combo2"), Center, CombatDataAsset->HammerFlow.Combo2MaxRange, DistanceToTarget);
		DrawMaxRange(TEXT("Finisher"), Center, CombatDataAsset->HammerFlow.FinisherMaxRange, DistanceToTarget);
		DrawMaxRange(TEXT("SprintHit"), Center, CombatDataAsset->SprintHammerHitConfig.AttackRange, DistanceToTarget);
	}

	const FPRPatternRule* SprintHammerRule = FindSprintHammerRule(GetPatternDataAsset());
	if (SprintHammerRule != nullptr)
	{
		const bool bInSprintRange = DistanceToTarget >= SprintHammerRule->MinRange
			&& DistanceToTarget <= SprintHammerRule->MaxRange;
		const FColor SprintColor = bInSprintRange ? SprintActiveColor : InactiveColor;

		DrawRangeCircle(Center, SprintHammerRule->MinRange, SprintColor);
		DrawRangeCircle(Center, SprintHammerRule->MaxRange, SprintColor);

		if (bDrawLabels)
		{
			DrawRangeLabel(TEXT("SprintMin"), Center, SprintHammerRule->MinRange, SprintColor);
			DrawRangeLabel(TEXT("SprintMax"), Center, SprintHammerRule->MaxRange, SprintColor);
		}
	}
}

void UPRSoldierArmoredDebugDrawComponent::DrawMaxRange(const FString& Label, const FVector& Center, float Radius, float DistanceToTarget) const
{
	if (Radius <= 0.0f)
	{
		return;
	}

	const bool bInRange = DistanceToTarget <= Radius;
	const FColor RangeColor = bInRange ? ActiveColor : InactiveColor;
	DrawRangeCircle(Center, Radius, RangeColor);

	if (bDrawLabels)
	{
		DrawRangeLabel(Label, Center, Radius, RangeColor);
	}
}

void UPRSoldierArmoredDebugDrawComponent::DrawRangeCircle(const FVector& Center, float Radius, const FColor& Color) const
{
	UWorld* World = GetWorld();
	if (!IsValid(World) || Radius <= 0.0f || CircleSegments < 3)
	{
		return;
	}

	FVector PreviousPoint = Center + FVector(Radius, 0.0f, 0.0f);
	for (int32 SegmentIndex = 1; SegmentIndex <= CircleSegments; ++SegmentIndex)
	{
		const float SegmentRatio = static_cast<float>(SegmentIndex) / static_cast<float>(CircleSegments);
		const float Angle = SegmentRatio * 2.0f * UE_PI;
		const FVector CurrentPoint = Center + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.0f);

		DrawDebugLine(World, PreviousPoint, CurrentPoint, Color, false, DrawDuration, 0, LineThickness);
		PreviousPoint = CurrentPoint;
	}
}

void UPRSoldierArmoredDebugDrawComponent::DrawRangeLabel(const FString& Label, const FVector& Center, float Radius, const FColor& Color) const
{
	UWorld* World = GetWorld();
	if (!IsValid(World) || Radius <= 0.0f)
	{
		return;
	}

	const FVector LabelLocation = Center + FVector(Radius, 0.0f, 48.0f);
	DrawDebugString(World, LabelLocation, FString::Printf(TEXT("%s %.0f"), *Label, Radius), GetOwner(), Color, DrawDuration, true);
}

// ===== 데이터 조회 =====

AActor* UPRSoldierArmoredDebugDrawComponent::GetCurrentTarget() const
{
	const IPREnemyInterface* EnemyInterface = Cast<IPREnemyInterface>(GetOwner());
	const UPREnemyThreatComponent* ThreatComponent = EnemyInterface != nullptr
		? EnemyInterface->GetEnemyThreatComponent()
		: nullptr;

	return IsValid(ThreatComponent)
		? ThreatComponent->GetCurrentTarget()
		: nullptr;
}

const UPRPatternDataAsset* UPRSoldierArmoredDebugDrawComponent::GetPatternDataAsset() const
{
	if (IsValid(PatternDataAssetOverride))
	{
		return PatternDataAssetOverride;
	}

	const IPREnemyInterface* EnemyInterface = Cast<IPREnemyInterface>(GetOwner());
	return EnemyInterface != nullptr
		? EnemyInterface->GetPatternDataAsset()
		: nullptr;
}

const FPRPatternRule* UPRSoldierArmoredDebugDrawComponent::FindSprintHammerRule(const UPRPatternDataAsset* PatternDataAsset) const
{
	if (!IsValid(PatternDataAsset))
	{
		return nullptr;
	}

	for (const FPRPatternRule& PatternRule : PatternDataAsset->PatternRules)
	{
		if (PatternRule.AbilityTag == PRSoldierArmoredGameplayTags::Ability_Enemy_SoldierArmored_SprintHammer)
		{
			return &PatternRule;
		}
	}

	return nullptr;
}

float UPRSoldierArmoredDebugDrawComponent::GetDistanceToTarget(const AActor* CurrentTarget) const
{
	const AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor) || !IsValid(CurrentTarget))
	{
		return TNumericLimits<float>::Max();
	}

	return FVector::Dist(OwnerActor->GetActorLocation(), CurrentTarget->GetActorLocation());
}

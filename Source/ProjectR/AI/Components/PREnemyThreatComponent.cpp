// Copyright ProjectR. All Rights Reserved.

#include "PREnemyThreatComponent.h"

#include "AbilitySystemComponent.h"
#include "ProjectR/AI/PREnemyAIDebug.h"
#include "ProjectR/Combat/PRCombatEngagementSubsystem.h"
#include "ProjectR/Combat/PRCombatStatics.h"
#include "ProjectR/PRGameplayTags.h"

#include "DrawDebugHelpers.h"

namespace
{
	constexpr float MinTargetSelectionScore = UE_SMALL_NUMBER;
	constexpr float TargetDebugOwnerHeightOffset = 92.0f;
	constexpr float TargetDebugTargetHeightOffset = 88.0f;
	constexpr float TargetDebugLabelHeightOffset = 132.0f;
	constexpr float TargetDebugLineThickness = 2.0f;
	constexpr float TargetDebugSphereRadius = 24.0f;

	FPREnemyTargetingConfig SanitizeTargetingConfig(FPREnemyTargetingConfig Config)
	{
		Config.ScoreWindowDuration = FMath::Max(Config.ScoreWindowDuration, 0.1f);
		Config.BaseCandidateScore = FMath::Max(Config.BaseCandidateScore, 0.0f);
		Config.DamageScoreScale = FMath::Max(Config.DamageScoreScale, 0.0f);
		Config.MaxDamageScore = FMath::Max(Config.MaxDamageScore, 0.0f);
		Config.DistanceScoreMax = FMath::Max(Config.DistanceScoreMax, 0.0f);
		Config.DistanceScoreMaxRange = FMath::Max(Config.DistanceScoreMaxRange, 1.0f);
		Config.LOSScore = FMath::Max(Config.LOSScore, 0.0f);
		Config.CurrentTargetStickinessScore = FMath::Max(Config.CurrentTargetStickinessScore, 0.0f);
		Config.SwitchScoreRatio = FMath::Max(Config.SwitchScoreRatio, 1.0f);
		Config.SwitchCooldown = FMath::Max(Config.SwitchCooldown, 0.0f);
		Config.EngagementRetainRadius = FMath::Max(Config.EngagementRetainRadius, 0.0f);
		Config.CandidateForgetTime = FMath::Max(Config.CandidateForgetTime, 0.0f);
		return Config;
	}

	const TCHAR* BoolToText(bool bValue)
	{
		return bValue ? TEXT("true") : TEXT("false");
	}

	FColor GetCandidateDebugColor(const FPREnemyTargetCandidate& Candidate, const AActor* CurrentTarget, const AActor* ActiveAttackTarget, const AActor* PendingTarget)
	{
		const AActor* CandidateTarget = Candidate.Target.Get();
		if (CandidateTarget == ActiveAttackTarget)
		{
			return FColor::Red;
		}

		if (CandidateTarget == PendingTarget)
		{
			return FColor(255, 128, 0);
		}

		if (CandidateTarget == CurrentTarget)
		{
			return FColor::Green;
		}

		if (Candidate.bCurrentlyPerceived)
		{
			return FColor::Cyan;
		}

		return FColor(150, 150, 150);
	}
}

UPREnemyThreatComponent::UPREnemyThreatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}

void UPREnemyThreatComponent::BeginPlay()
{
	Super::BeginPlay();
	StartCombatEngagementReportTimer();
}

void UPREnemyThreatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopCombatEngagementReportTimer();
	Super::EndPlay(EndPlayReason);
}

AActor* UPREnemyThreatComponent::GetAttackTarget() const
{
	if (AttackCommitState.bIsAttackCommitted && IsValidThreatTarget(AttackCommitState.ActiveAttackTarget))
	{
		return AttackCommitState.ActiveAttackTarget;
	}

	return IsValidThreatTarget(CurrentTarget) ? CurrentTarget.Get() : nullptr;
}

bool UPREnemyThreatComponent::CanSwitchCurrentTarget() const
{
	return !AttackCommitState.bIsAttackCommitted;
}

void UPREnemyThreatComponent::SetTargetingConfig(const FPREnemyTargetingConfig& InTargetingConfig)
{
	TargetingConfig = SanitizeTargetingConfig(InTargetingConfig);
}

void UPREnemyThreatComponent::AddThreat(AActor* Target, float Amount)
{
	// 타겟 선택은 서버에서만 결정한다. 클라이언트는 결과를 Blackboard/복제로 따라간다.
	if (!GetOwner() || !GetOwner()->HasAuthority() || !IsValidThreatTarget(Target) || Amount <= 0.0f)
	{
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	ResetScoreWindowIfNeeded(CurrentTime);
	FPREnemyTargetCandidate& Candidate = FindOrAddTargetCandidate(Target, CurrentTime);

	// 기존 Threat 선택 동작을 보존하면서 이후 점수 기반 Targeting으로 확장할 수 있게 후보 값을 함께 갱신한다.
	Candidate.ThreatValue += Amount;
	Candidate.DamageInCurrentWindow += Amount;
	Candidate.LastThreatTime = CurrentTime;
	Candidate.LastUpdatedTime = CurrentTime;
	Candidate.LastKnownLocation = Target->GetActorLocation();
	UpdateCandidateSelectionScore(Candidate);

	ReevaluateTarget();
	LogTargetDebugState(TEXT("AddThreat"));
	DrawTargetDebugState(TEXT("AddThreat"));
}

void UPREnemyThreatComponent::AddDamageThreat(AActor* Target, float DamageAmount)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !IsValidThreatTarget(Target) || DamageAmount <= 0.0f)
	{
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	ResetScoreWindowIfNeeded(CurrentTime);
	FPREnemyTargetCandidate& Candidate = FindOrAddTargetCandidate(Target, CurrentTime);

	Candidate.DamageInCurrentWindow += DamageAmount;
	Candidate.ThreatValue += DamageAmount;
	Candidate.LastThreatTime = CurrentTime;
	Candidate.LastUpdatedTime = CurrentTime;
	Candidate.LastKnownLocation = Target->GetActorLocation();
	UpdateCandidateSelectionScore(Candidate);

	ForceCurrentTarget(Target);
	LogTargetDebugState(TEXT("AddDamageThreat"));
	DrawTargetDebugState(TEXT("AddDamageThreat"));
}

void UPREnemyThreatComponent::ForceCurrentTarget(AActor* NewTarget)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !IsValidThreatTarget(NewTarget))
	{
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	ResetScoreWindowIfNeeded(CurrentTime);
	FPREnemyTargetCandidate& Candidate = FindOrAddTargetCandidate(NewTarget, CurrentTime);

	Candidate.LastKnownLocation = NewTarget->GetActorLocation();
	Candidate.LastUpdatedTime = CurrentTime;
	const float CandidateScore = UpdateCandidateSelectionScore(Candidate);

	SetCurrentTarget(NewTarget, CandidateScore);
	LogTargetDebugState(TEXT("ForceCurrentTarget"));
	DrawTargetDebugState(TEXT("ForceCurrentTarget"));
}

void UPREnemyThreatComponent::UpdatePerceivedTarget(AActor* Target, FVector SensedLocation, bool bHasLOS)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !IsValidThreatTarget(Target))
	{
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	ResetScoreWindowIfNeeded(CurrentTime);
	FPREnemyTargetCandidate& Candidate = FindOrAddTargetCandidate(Target, CurrentTime);
	const FVector ResolvedLocation = SensedLocation.IsNearlyZero() ? Target->GetActorLocation() : SensedLocation;

	Candidate.bCurrentlyPerceived = true;
	Candidate.bHasLOS = bHasLOS;
	Candidate.LastKnownLocation = ResolvedLocation;
	Candidate.LastSensedTime = CurrentTime;
	Candidate.LastUpdatedTime = CurrentTime;
	Candidate.BaseScore = TargetingConfig.BaseCandidateScore;
	Candidate.ThreatValue = FMath::Max(Candidate.ThreatValue, TargetingConfig.BaseCandidateScore);
	UpdateCandidateSelectionScore(Candidate);

	// 이미 공격 대상이 있으면 새 감지 대상은 후보 풀에만 추가하고, 교체는 점수창 갱신/피해 이벤트에서 처리한다.
	if (!IsValid(CurrentTarget))
	{
		ReevaluateTarget();
	}

	LogTargetDebugState(TEXT("UpdatePerceivedTarget"));
	DrawTargetDebugState(TEXT("UpdatePerceivedTarget"));
}

void UPREnemyThreatComponent::MarkTargetPerceptionLost(AActor* LostTarget, FVector LastKnownLocation)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !IsValidThreatTarget(LostTarget))
	{
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	ResetScoreWindowIfNeeded(CurrentTime);
	FPREnemyTargetCandidate* Candidate = FindTargetCandidate(LostTarget);
	if (Candidate == nullptr)
	{
		return;
	}

	const FVector ResolvedLocation = LastKnownLocation.IsNearlyZero() ? LostTarget->GetActorLocation() : LastKnownLocation;

	Candidate->bCurrentlyPerceived = false;
	Candidate->bHasLOS = false;
	Candidate->LastKnownLocation = ResolvedLocation;
	Candidate->LastUpdatedTime = CurrentTime;

	RefreshTargetCandidates();
	LogTargetDebugState(TEXT("MarkTargetPerceptionLost"));
	DrawTargetDebugState(TEXT("MarkTargetPerceptionLost"));
}

bool UPREnemyThreatComponent::GetTargetCandidate(AActor* Target, FPREnemyTargetCandidate& OutCandidate) const
{
	const FPREnemyTargetCandidate* Candidate = FindTargetCandidate(Target);
	if (Candidate == nullptr)
	{
		return false;
	}

	OutCandidate = *Candidate;
	return true;
}

void UPREnemyThreatComponent::RefreshTargetCandidates()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	const bool bScoreWindowReset = ResetScoreWindowIfNeeded(CurrentTime, false);
	CleanupInvalidEntries();

	for (FPREnemyTargetCandidate& Candidate : TargetCandidates)
	{
		UpdateCandidateSelectionScore(Candidate);
	}

	if (IsValid(CurrentTarget) && FindTargetCandidate(CurrentTarget) == nullptr)
	{
		SetCurrentTarget(nullptr);
	}

	if (bScoreWindowReset || !IsValid(CurrentTarget))
	{
		ReevaluateTarget(false);
	}

	if (bScoreWindowReset)
	{
		ClearScoreWindowDamage();
		for (FPREnemyTargetCandidate& Candidate : TargetCandidates)
		{
			UpdateCandidateSelectionScore(Candidate);
		}

		LogTargetDebugState(TEXT("ScoreWindowReset"));
	}

	DrawTargetDebugState(TEXT("RefreshTargetCandidates"));
}

void UPREnemyThreatComponent::BeginAttackCommit(AActor* InTarget, FVector InTargetLocation)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	AActor* CommitTarget = IsValid(InTarget) ? InTarget : CurrentTarget.Get();
	if (!IsValidThreatTarget(CommitTarget))
	{
		return;
	}

	AttackCommitState.ActiveAttackTarget = CommitTarget;
	AttackCommitState.ActiveAttackTargetLocation = InTargetLocation;
	AttackCommitState.bIsAttackCommitted = true;
	ClearPendingTarget();
	LogTargetDebugState(TEXT("BeginAttackCommit"));
	DrawTargetDebugState(TEXT("BeginAttackCommit"));
}

void UPREnemyThreatComponent::EndAttackCommit()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	if (!AttackCommitState.bIsAttackCommitted && !IsValid(AttackCommitState.PendingTargetCandidate))
	{
		return;
	}

	AActor* PendingTarget = AttackCommitState.PendingTargetCandidate;
	const float PendingScore = AttackCommitState.PendingTargetScore;

	AttackCommitState.ActiveAttackTarget = nullptr;
	AttackCommitState.ActiveAttackTargetLocation = FVector::ZeroVector;
	AttackCommitState.bIsAttackCommitted = false;
	ClearPendingTarget();

	if (IsValidThreatTarget(PendingTarget))
	{
		SetCurrentTarget(PendingTarget, PendingScore);
		LogTargetDebugState(TEXT("EndAttackCommit-Pending"));
		DrawTargetDebugState(TEXT("EndAttackCommit-Pending"));
		return;
	}

	ReevaluateTarget();
	LogTargetDebugState(TEXT("EndAttackCommit"));
	DrawTargetDebugState(TEXT("EndAttackCommit"));
}

void UPREnemyThreatComponent::ForceClearAttackCommit()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	AttackCommitState.ActiveAttackTarget = nullptr;
	AttackCommitState.ActiveAttackTargetLocation = FVector::ZeroVector;
	AttackCommitState.bIsAttackCommitted = false;
	ClearPendingTarget();
	LogTargetDebugState(TEXT("ForceClearAttackCommit"));
	DrawTargetDebugState(TEXT("ForceClearAttackCommit"));
}

void UPREnemyThreatComponent::InvalidateCurrentTarget()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	SetCurrentTarget(nullptr);
	ReevaluateTarget();
}

void UPREnemyThreatComponent::ReleaseCurrentTargetForSearch(AActor* LostTarget)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !IsValid(LostTarget))
	{
		return;
	}

	if (CurrentTarget == LostTarget)
	{
		// Threat 기록은 남겨두되 현재 타겟만 비워 마지막 위치 수색 브랜치가 실행되도록 한다.
		SetCurrentTarget(nullptr);
	}

	for (FPREnemyTargetCandidate& Entry : TargetCandidates)
	{
		if (Entry.Target == LostTarget)
		{
			// 재감지되면 AddThreat가 시간을 갱신한다. 재감지되지 않으면 다음 재평가 때 자연스럽게 정리된다.
			Entry.bCurrentlyPerceived = false;
			Entry.bHasLOS = false;
			Entry.LastUpdatedTime = GetWorld()->GetTimeSeconds() - TargetingConfig.CandidateForgetTime;
			break;
		}
	}

	LogTargetDebugState(TEXT("ReleaseCurrentTargetForSearch"));
	DrawTargetDebugState(TEXT("ReleaseCurrentTargetForSearch"));
}

void UPREnemyThreatComponent::OnTargetLost(AActor* LostTarget)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !IsValid(LostTarget))
	{
		return;
	}

	for (int32 Index = TargetCandidates.Num() - 1; Index >= 0; --Index)
	{
		if (TargetCandidates[Index].Target == LostTarget)
		{
			TargetCandidates.RemoveAt(Index);
		}
	}

	if (CurrentTarget == LostTarget)
	{
		SetCurrentTarget(nullptr);
	}

	ReevaluateTarget();
	LogTargetDebugState(TEXT("OnTargetLost"));
	DrawTargetDebugState(TEXT("OnTargetLost"));
}

void UPREnemyThreatComponent::StartCombatEngagementReportTimer()
{
	AActor* OwnerActor = GetOwner();
	UWorld* World = GetWorld();
	if (!IsValid(OwnerActor) || !OwnerActor->HasAuthority() || !IsValid(World))
	{
		return;
	}

	const float SanitizedInterval = FMath::Max(CombatEngagementReportInterval, 0.1f);
	World->GetTimerManager().SetTimer(
		CombatEngagementReportTimerHandle,
		this,
		&UPREnemyThreatComponent::ReportCombatEngagedCandidates,
		SanitizedInterval,
		true,
		FMath::FRandRange(0.0f, SanitizedInterval));
}

void UPREnemyThreatComponent::StopCombatEngagementReportTimer()
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	World->GetTimerManager().ClearTimer(CombatEngagementReportTimerHandle);
}

void UPREnemyThreatComponent::ReportCombatEngagedCandidates()
{
	AActor* OwnerActor = GetOwner();
	UWorld* World = GetWorld();
	if (!IsValid(OwnerActor) || !OwnerActor->HasAuthority() || !IsValid(World) || TargetCandidates.IsEmpty())
	{
		return;
	}

	UPRCombatEngagementSubsystem* EngagementSubsystem = World->GetSubsystem<UPRCombatEngagementSubsystem>();
	if (!IsValid(EngagementSubsystem))
	{
		return;
	}

	for (const FPREnemyTargetCandidate& Candidate : TargetCandidates)
	{
		if (ShouldReportCombatEngagedCandidate(Candidate))
		{
			EngagementSubsystem->ReportCombatEngagedCandidate(OwnerActor, Candidate.Target.Get());
		}
	}
}

bool UPREnemyThreatComponent::ShouldReportCombatEngagedCandidate(const FPREnemyTargetCandidate& Candidate) const
{
	return IsValidThreatTarget(Candidate.Target);
}

void UPREnemyThreatComponent::ReevaluateTarget(bool bResetScoreWindow)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (bResetScoreWindow)
	{
		ResetScoreWindowIfNeeded(CurrentTime);
	}
	CleanupInvalidEntries();

	if (IsValid(CurrentTarget) && FindTargetCandidate(CurrentTarget) == nullptr)
	{
		SetCurrentTarget(nullptr);
	}

	for (FPREnemyTargetCandidate& Candidate : TargetCandidates)
	{
		UpdateCandidateSelectionScore(Candidate);
	}

	AActor* BestTarget = nullptr;
	float BestThreat = 0.0f;
	if (!SelectWeightedTarget(BestTarget, BestThreat))
	{
		SetCurrentTarget(nullptr);
		return;
	}

	if (!IsValid(BestTarget))
	{
		SetCurrentTarget(nullptr);
		return;
	}

	if (!IsValid(CurrentTarget))
	{
		SetCurrentTarget(BestTarget, BestThreat);
		return;
	}

	if (CurrentTarget == BestTarget)
	{
		return;
	}

	// 현재 타겟을 너무 쉽게 바꾸면 몬스터가 산만해지므로,
	// 큰 점수 차이나 최소 전환 간격 중 하나를 만족할 때만 교체한다.
	const bool bCanSwitchByTime = (CurrentTime - LastSwitchTime) >= TargetingConfig.SwitchCooldown;
	const FPREnemyTargetCandidate* CurrentTargetCandidate = FindTargetCandidate(CurrentTarget);
	const float CurrentTargetScore = CurrentTargetCandidate != nullptr
		? CurrentTargetCandidate->FinalSelectionScore
		: 0.0f;
	const bool bCanSwitchByScore = CurrentTargetScore <= MinTargetSelectionScore
		|| BestThreat >= CurrentTargetScore * TargetingConfig.SwitchScoreRatio;

	if (AttackCommitState.bIsAttackCommitted)
	{
		SetCurrentTarget(BestTarget, BestThreat);
		return;
	}

	if (bCanSwitchByTime || bCanSwitchByScore)
	{
		SetCurrentTarget(BestTarget, BestThreat);
	}
}

bool UPREnemyThreatComponent::IsValidThreatTarget(const AActor* Target) const
{
	if (!IsValid(Target) || Target == GetOwner())
	{
		return false;
	}

	// 일반 몬스터 AI는 플레이어만 공격 대상으로 추적한다.
	if (UPRCombatStatics::GetActorTeam(Target) != EPRTeam::Player)
	{
		return false;
	}

	const UAbilitySystemComponent* TargetAbilitySystem = UPRCombatStatics::FindAbilitySystemComponent(Target);
	if (!IsValid(TargetAbilitySystem))
	{
		return false;
	}

	// 다운/사망한 플레이어는 감지되어도 공격 후보와 현재 공격 대상으로 유지하지 않는다.
	return !TargetAbilitySystem->HasMatchingGameplayTag(PRGameplayTags::State_Down)
		&& !TargetAbilitySystem->HasMatchingGameplayTag(PRGameplayTags::State_Dead);
}

void UPREnemyThreatComponent::SetCurrentTarget(AActor* NewTarget, float CandidateScore)
{
	if (CurrentTarget == NewTarget)
	{
		return;
	}

	if (!CanSwitchCurrentTarget() && IsValid(NewTarget))
	{
		QueuePendingTarget(NewTarget, CandidateScore);
		return;
	}

	AActor* OldTarget = CurrentTarget;
	CurrentTarget = NewTarget;
	LastSwitchTime = GetWorld()->GetTimeSeconds();

	if (PREnemyAIDebug::IsTargetingLogEnabled())
	{
		UE_LOG(LogPREnemyAI, Log, TEXT("[Targeting][SetCurrentTarget] Owner=%s Old=%s New=%s Score=%.2f"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(OldTarget),
			*GetNameSafe(NewTarget),
			CandidateScore);
	}

	if (!IsValid(NewTarget) || AttackCommitState.PendingTargetCandidate == NewTarget)
	{
		ClearPendingTarget();
	}

	OnTargetChanged.Broadcast(OldTarget, NewTarget);
	DrawTargetDebugState(TEXT("SetCurrentTarget"));
}

void UPREnemyThreatComponent::CleanupInvalidEntries()
{
	const float CurrentTime = GetWorld()->GetTimeSeconds();

	for (int32 Index = TargetCandidates.Num() - 1; Index >= 0; --Index)
	{
		if (ShouldRemoveTargetCandidate(TargetCandidates[Index], CurrentTime))
		{
			TargetCandidates.RemoveAt(Index);
		}
	}

	if (IsValid(CurrentTarget) && !IsValidThreatTarget(CurrentTarget))
	{
		SetCurrentTarget(nullptr);
	}

	if (IsValid(AttackCommitState.ActiveAttackTarget) && !IsValidThreatTarget(AttackCommitState.ActiveAttackTarget))
	{
		AttackCommitState.ActiveAttackTarget = nullptr;
		AttackCommitState.ActiveAttackTargetLocation = FVector::ZeroVector;
		AttackCommitState.bIsAttackCommitted = false;
	}

	if (IsValid(AttackCommitState.PendingTargetCandidate) && !IsValidThreatTarget(AttackCommitState.PendingTargetCandidate))
	{
		ClearPendingTarget();
	}
}

bool UPREnemyThreatComponent::ResetScoreWindowIfNeeded(float CurrentTime, bool bClearDamageScores)
{
	if (LastScoreWindowResetTime < 0.0f)
	{
		LastScoreWindowResetTime = CurrentTime;
		return false;
	}

	if ((CurrentTime - LastScoreWindowResetTime) < TargetingConfig.ScoreWindowDuration)
	{
		return false;
	}

	LastScoreWindowResetTime = CurrentTime;
	if (bClearDamageScores)
	{
		ClearScoreWindowDamage();
	}

	return true;
}

void UPREnemyThreatComponent::ClearScoreWindowDamage()
{
	for (FPREnemyTargetCandidate& Candidate : TargetCandidates)
	{
		Candidate.DamageInCurrentWindow = 0.0f;
		Candidate.DamageScore = 0.0f;
	}
}

float UPREnemyThreatComponent::UpdateCandidateSelectionScore(FPREnemyTargetCandidate& Candidate) const
{
	Candidate.BaseScore = FMath::Max(TargetingConfig.BaseCandidateScore, 0.0f);
	Candidate.DamageScore = FMath::Clamp(
		Candidate.DamageInCurrentWindow * TargetingConfig.DamageScoreScale,
		0.0f,
		TargetingConfig.MaxDamageScore);

	Candidate.DistanceScore = 0.0f;
	const AActor* OwnerActor = GetOwner();
	if (IsValid(OwnerActor) && IsValid(Candidate.Target) && TargetingConfig.DistanceScoreMaxRange > 0.0f)
	{
		const float DistanceToTarget = FVector::Dist(OwnerActor->GetActorLocation(), Candidate.Target->GetActorLocation());
		const float DistanceAlpha = 1.0f - FMath::Clamp(DistanceToTarget / TargetingConfig.DistanceScoreMaxRange, 0.0f, 1.0f);
		Candidate.DistanceScore = DistanceAlpha * FMath::Max(TargetingConfig.DistanceScoreMax, 0.0f);
	}

	Candidate.StickinessScore = Candidate.Target == CurrentTarget
		? FMath::Max(TargetingConfig.CurrentTargetStickinessScore, 0.0f)
		: 0.0f;

	const float LOSScore = Candidate.bHasLOS ? FMath::Max(TargetingConfig.LOSScore, 0.0f) : 0.0f;
	Candidate.FinalSelectionScore = FMath::Max(
		Candidate.BaseScore + Candidate.DamageScore + Candidate.DistanceScore + Candidate.StickinessScore + LOSScore,
		0.0f);
	Candidate.ThreatValue = Candidate.FinalSelectionScore;
	return Candidate.FinalSelectionScore;
}

bool UPREnemyThreatComponent::SelectWeightedTarget(AActor*& OutTarget, float& OutScore) const
{
	OutTarget = nullptr;
	OutScore = 0.0f;

	float TotalScore = 0.0f;
	for (const FPREnemyTargetCandidate& Candidate : TargetCandidates)
	{
		if (!IsValidThreatTarget(Candidate.Target) || Candidate.FinalSelectionScore <= MinTargetSelectionScore)
		{
			continue;
		}

		TotalScore += Candidate.FinalSelectionScore;
	}

	if (TotalScore <= MinTargetSelectionScore)
	{
		return false;
	}

	float Pick = FMath::FRandRange(0.0f, TotalScore);
	for (const FPREnemyTargetCandidate& Candidate : TargetCandidates)
	{
		if (!IsValidThreatTarget(Candidate.Target) || Candidate.FinalSelectionScore <= MinTargetSelectionScore)
		{
			continue;
		}

		Pick -= Candidate.FinalSelectionScore;
		if (Pick <= 0.0f)
		{
			OutTarget = Candidate.Target;
			OutScore = Candidate.FinalSelectionScore;
			return true;
		}
	}

	return false;
}

FPREnemyTargetCandidate* UPREnemyThreatComponent::FindTargetCandidate(AActor* Target)
{
	if (!IsValid(Target))
	{
		return nullptr;
	}

	for (FPREnemyTargetCandidate& Candidate : TargetCandidates)
	{
		if (Candidate.Target == Target)
		{
			return &Candidate;
		}
	}

	return nullptr;
}

const FPREnemyTargetCandidate* UPREnemyThreatComponent::FindTargetCandidate(AActor* Target) const
{
	if (!IsValid(Target))
	{
		return nullptr;
	}

	for (const FPREnemyTargetCandidate& Candidate : TargetCandidates)
	{
		if (Candidate.Target == Target)
		{
			return &Candidate;
		}
	}

	return nullptr;
}

FPREnemyTargetCandidate& UPREnemyThreatComponent::FindOrAddTargetCandidate(AActor* Target, float CurrentTime)
{
	if (FPREnemyTargetCandidate* ExistingCandidate = FindTargetCandidate(Target))
	{
		return *ExistingCandidate;
	}

	FPREnemyTargetCandidate& NewCandidate = TargetCandidates.AddDefaulted_GetRef();
	NewCandidate.Target = Target;
	NewCandidate.BaseScore = TargetingConfig.BaseCandidateScore;
	NewCandidate.LastKnownLocation = IsValid(Target) ? Target->GetActorLocation() : FVector::ZeroVector;
	NewCandidate.LastSensedTime = CurrentTime;
	NewCandidate.LastThreatTime = CurrentTime;
	NewCandidate.LastUpdatedTime = CurrentTime;
	NewCandidate.FinalSelectionScore = TargetingConfig.BaseCandidateScore;
	NewCandidate.ThreatValue = TargetingConfig.BaseCandidateScore;
	return NewCandidate;
}

bool UPREnemyThreatComponent::ShouldRemoveTargetCandidate(const FPREnemyTargetCandidate& Candidate, float CurrentTime) const
{
	if (!IsValidThreatTarget(Candidate.Target))
	{
		return true;
	}

	if (Candidate.bCurrentlyPerceived)
	{
		return false;
	}

	const AActor* OwnerActor = GetOwner();
	if (IsValid(OwnerActor) && TargetingConfig.EngagementRetainRadius > 0.0f)
	{
		const float DistanceToTarget = FVector::Dist(OwnerActor->GetActorLocation(), Candidate.Target->GetActorLocation());
		return DistanceToTarget > TargetingConfig.EngagementRetainRadius;
	}

	return (CurrentTime - Candidate.LastUpdatedTime) > TargetingConfig.CandidateForgetTime;
}

void UPREnemyThreatComponent::QueuePendingTarget(AActor* NewTarget, float CandidateScore)
{
	if (!IsValidThreatTarget(NewTarget))
	{
		return;
	}

	AttackCommitState.PendingTargetCandidate = NewTarget;
	AttackCommitState.PendingTargetScore = CandidateScore;

	if (PREnemyAIDebug::IsTargetingLogEnabled())
	{
		UE_LOG(LogPREnemyAI, Log, TEXT("[Targeting][QueuePendingTarget] Owner=%s Pending=%s Score=%.2f Active=%s Current=%s"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(NewTarget),
			CandidateScore,
			*GetNameSafe(AttackCommitState.ActiveAttackTarget.Get()),
			*GetNameSafe(CurrentTarget.Get()));
	}
}

void UPREnemyThreatComponent::ClearPendingTarget()
{
	if (PREnemyAIDebug::IsTargetingLogEnabled() && IsValid(AttackCommitState.PendingTargetCandidate))
	{
		UE_LOG(LogPREnemyAI, Log, TEXT("[Targeting][ClearPendingTarget] Owner=%s Pending=%s Score=%.2f"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(AttackCommitState.PendingTargetCandidate.Get()),
			AttackCommitState.PendingTargetScore);
	}

	AttackCommitState.PendingTargetCandidate = nullptr;
	AttackCommitState.PendingTargetScore = 0.0f;
}

void UPREnemyThreatComponent::LogTargetDebugState(const TCHAR* Reason) const
{
	if (!PREnemyAIDebug::IsTargetingLogEnabled())
	{
		return;
	}

	const AActor* OwnerActor = GetOwner();
	const UWorld* World = GetWorld();
	const float CurrentTime = World != nullptr ? World->GetTimeSeconds() : 0.0f;
	const TCHAR* DebugReason = Reason != nullptr ? Reason : TEXT("Unknown");

	UE_LOG(LogPREnemyAI, Log, TEXT("[Targeting][%s] Owner=%s Current=%s AttackTarget=%s Active=%s Pending=%s PendingScore=%.2f Committed=%s Candidates=%d"),
		DebugReason,
		*GetNameSafe(OwnerActor),
		*GetNameSafe(CurrentTarget.Get()),
		*GetNameSafe(GetAttackTarget()),
		*GetNameSafe(AttackCommitState.ActiveAttackTarget.Get()),
		*GetNameSafe(AttackCommitState.PendingTargetCandidate.Get()),
		AttackCommitState.PendingTargetScore,
		BoolToText(AttackCommitState.bIsAttackCommitted),
		TargetCandidates.Num());

	for (int32 Index = 0; Index < TargetCandidates.Num(); ++Index)
	{
		const FPREnemyTargetCandidate& Candidate = TargetCandidates[Index];
		const AActor* CandidateTarget = Candidate.Target.Get();
		const float DistanceToOwner = IsValid(OwnerActor) && IsValid(CandidateTarget)
			? FVector::Dist(OwnerActor->GetActorLocation(), CandidateTarget->GetActorLocation())
			: -1.0f;
		const float UpdatedAge = CurrentTime - Candidate.LastUpdatedTime;
		const float SensedAge = CurrentTime - Candidate.LastSensedTime;

		UE_LOG(LogPREnemyAI, Log, TEXT("  [%d] Target=%s Final=%.2f Base=%.2f Damage=%.2f Distance=%.2f Stickiness=%.2f Threat=%.2f DamageWindow=%.2f DistToOwner=%.1f Perceived=%s LOS=%s UpdatedAge=%.2f SensedAge=%.2f LastKnown=%s"),
			Index,
			*GetNameSafe(CandidateTarget),
			Candidate.FinalSelectionScore,
			Candidate.BaseScore,
			Candidate.DamageScore,
			Candidate.DistanceScore,
			Candidate.StickinessScore,
			Candidate.ThreatValue,
			Candidate.DamageInCurrentWindow,
			DistanceToOwner,
			BoolToText(Candidate.bCurrentlyPerceived),
			BoolToText(Candidate.bHasLOS),
			UpdatedAge,
			SensedAge,
			*Candidate.LastKnownLocation.ToCompactString());
	}
}

void UPREnemyThreatComponent::DrawTargetDebugState(const TCHAR* Reason) const
{
	if (!PREnemyAIDebug::IsTargetingDrawEnabled())
	{
		return;
	}

	UWorld* World = GetWorld();
	const AActor* OwnerActor = GetOwner();
	if (World == nullptr || !IsValid(OwnerActor))
	{
		return;
	}

	const FVector OwnerLocation = OwnerActor->GetActorLocation() + FVector(0.0f, 0.0f, TargetDebugOwnerHeightOffset);
	const float DrawDuration = PREnemyAIDebug::GetTargetingDrawDuration();
	const AActor* ActiveAttackTarget = AttackCommitState.ActiveAttackTarget.Get();
	const AActor* PendingTarget = AttackCommitState.PendingTargetCandidate.Get();
	const TCHAR* DebugReason = Reason != nullptr ? Reason : TEXT("Unknown");

	const FString OwnerLabel = FString::Printf(TEXT("TargetDebug: %s\nCurrent=%s Attack=%s Commit=%s Candidates=%d"),
		DebugReason,
		*GetNameSafe(CurrentTarget.Get()),
		*GetNameSafe(GetAttackTarget()),
		BoolToText(AttackCommitState.bIsAttackCommitted),
		TargetCandidates.Num());
	DrawDebugString(World, OwnerLocation + FVector(0.0f, 0.0f, TargetDebugLabelHeightOffset), OwnerLabel, nullptr, FColor::White, DrawDuration, true, 1.0f);

	for (int32 Index = 0; Index < TargetCandidates.Num(); ++Index)
	{
		const FPREnemyTargetCandidate& Candidate = TargetCandidates[Index];
		const AActor* CandidateTarget = Candidate.Target.Get();
		if (!IsValid(CandidateTarget))
		{
			continue;
		}

		const FColor CandidateColor = GetCandidateDebugColor(Candidate, CurrentTarget.Get(), ActiveAttackTarget, PendingTarget);
		const FVector TargetLocation = CandidateTarget->GetActorLocation() + FVector(0.0f, 0.0f, TargetDebugTargetHeightOffset);
		DrawDebugLine(World, OwnerLocation, TargetLocation, CandidateColor, false, DrawDuration, 0, TargetDebugLineThickness);
		DrawDebugSphere(World, TargetLocation, TargetDebugSphereRadius, 12, CandidateColor, false, DrawDuration, 0, TargetDebugLineThickness);

		const FString CandidateLabel = FString::Printf(TEXT("[%d] %s\nScore %.2f Dmg %.2f Dist %.2f Stick %.2f\nPerceived=%s LOS=%s Last=%s"),
			Index,
			*GetNameSafe(CandidateTarget),
			Candidate.FinalSelectionScore,
			Candidate.DamageScore,
			Candidate.DistanceScore,
			Candidate.StickinessScore,
			BoolToText(Candidate.bCurrentlyPerceived),
			BoolToText(Candidate.bHasLOS),
			*Candidate.LastKnownLocation.ToCompactString());
		DrawDebugString(World, TargetLocation + FVector(0.0f, 0.0f, TargetDebugLabelHeightOffset + static_cast<float>(Index) * 18.0f), CandidateLabel, nullptr, CandidateColor, DrawDuration, true, 0.9f);
	}

	if (IsValid(ActiveAttackTarget))
	{
		const FVector ActiveLocation = ActiveAttackTarget->GetActorLocation() + FVector(0.0f, 0.0f, TargetDebugTargetHeightOffset + 48.0f);
		DrawDebugSphere(World, ActiveLocation, TargetDebugSphereRadius * 1.5f, 16, FColor::Red, false, DrawDuration, 0, TargetDebugLineThickness + 1.0f);
	}
}

// Copyright ProjectR. All Rights Reserved.

#include "PREnemyThreatComponent.h"

#include "ProjectR/Combat/PRCombatStatics.h"

UPREnemyThreatComponent::UPREnemyThreatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}

AActor* UPREnemyThreatComponent::GetAttackTarget() const
{
	if (AttackCommitState.bIsAttackCommitted && IsValid(AttackCommitState.ActiveAttackTarget))
	{
		return AttackCommitState.ActiveAttackTarget;
	}

	return CurrentTarget;
}

bool UPREnemyThreatComponent::CanSwitchCurrentTarget() const
{
	return !AttackCommitState.bIsAttackCommitted;
}

void UPREnemyThreatComponent::AddThreat(AActor* Target, float Amount)
{
	// 타겟 선택은 서버에서만 결정한다. 클라이언트는 결과를 Blackboard/복제로 따라간다.
	if (!GetOwner() || !GetOwner()->HasAuthority() || !IsValidThreatTarget(Target) || Amount <= 0.0f)
	{
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	FPREnemyTargetCandidate& Candidate = FindOrAddTargetCandidate(Target, CurrentTime);

	// 기존 Threat 선택 동작을 보존하면서 이후 점수 기반 Targeting으로 확장할 수 있게 후보 값을 함께 갱신한다.
	Candidate.ThreatValue += Amount;
	Candidate.FinalSelectionScore = Candidate.ThreatValue;
	Candidate.LastThreatTime = CurrentTime;
	Candidate.LastUpdatedTime = CurrentTime;
	Candidate.LastKnownLocation = Target->GetActorLocation();

	ReevaluateTarget();
}

void UPREnemyThreatComponent::UpdatePerceivedTarget(AActor* Target, FVector SensedLocation, bool bHasLOS)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !IsValidThreatTarget(Target))
	{
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	FPREnemyTargetCandidate& Candidate = FindOrAddTargetCandidate(Target, CurrentTime);
	const FVector ResolvedLocation = SensedLocation.IsNearlyZero() ? Target->GetActorLocation() : SensedLocation;

	Candidate.bCurrentlyPerceived = true;
	Candidate.bHasLOS = bHasLOS;
	Candidate.LastKnownLocation = ResolvedLocation;
	Candidate.LastSensedTime = CurrentTime;
	Candidate.LastUpdatedTime = CurrentTime;
	Candidate.BaseScore = TargetingConfig.BaseCandidateScore;
	Candidate.ThreatValue = FMath::Max(Candidate.ThreatValue, TargetingConfig.BaseCandidateScore);
	Candidate.FinalSelectionScore = Candidate.ThreatValue;

	ReevaluateTarget();
}

void UPREnemyThreatComponent::MarkTargetPerceptionLost(AActor* LostTarget, FVector LastKnownLocation)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !IsValidThreatTarget(LostTarget))
	{
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	FPREnemyTargetCandidate& Candidate = FindOrAddTargetCandidate(LostTarget, CurrentTime);
	const FVector ResolvedLocation = LastKnownLocation.IsNearlyZero() ? LostTarget->GetActorLocation() : LastKnownLocation;

	Candidate.bCurrentlyPerceived = false;
	Candidate.bHasLOS = false;
	Candidate.LastKnownLocation = ResolvedLocation;
	Candidate.LastUpdatedTime = CurrentTime;

	RefreshTargetCandidates();
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

	ReevaluateTarget();
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
}

void UPREnemyThreatComponent::EndAttackCommit()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
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
		return;
	}

	ReevaluateTarget();
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
}

void UPREnemyThreatComponent::ReevaluateTarget()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	CleanupInvalidEntries();

	if (IsValid(CurrentTarget) && FindTargetCandidate(CurrentTarget) == nullptr)
	{
		SetCurrentTarget(nullptr);
	}

	AActor* BestTarget = nullptr;
	float BestThreat = 0.0f;
	float CurrentThreat = 0.0f;

	for (const FPREnemyTargetCandidate& Entry : TargetCandidates)
	{
		if (!IsValidThreatTarget(Entry.Target))
		{
			continue;
		}

		if (Entry.Target == CurrentTarget)
		{
			CurrentThreat = Entry.ThreatValue;
		}

		if (Entry.ThreatValue > BestThreat)
		{
			BestThreat = Entry.ThreatValue;
			BestTarget = Entry.Target;
		}
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
	// 위협 차이와 최소 전환 간격을 동시에 만족할 때만 교체한다.
	const float CurrentTime = GetWorld()->GetTimeSeconds();
	const bool bCanSwitchByRatio = BestThreat >= (CurrentThreat * TargetingConfig.SwitchScoreRatio);
	const bool bCanSwitchByTime = (CurrentTime - LastSwitchTime) >= TargetingConfig.SwitchCooldown;

	if (bCanSwitchByRatio && bCanSwitchByTime)
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
	return UPRCombatStatics::GetActorTeam(Target) == EPRTeam::Player;
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

	if (!IsValid(NewTarget) || AttackCommitState.PendingTargetCandidate == NewTarget)
	{
		ClearPendingTarget();
	}

	OnTargetChanged.Broadcast(OldTarget, NewTarget);
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
}

void UPREnemyThreatComponent::ClearPendingTarget()
{
	AttackCommitState.PendingTargetCandidate = nullptr;
	AttackCommitState.PendingTargetScore = 0.0f;
}

// Copyright ProjectR. All Rights Reserved.

#include "PREnemyThreatComponent.h"

UPREnemyThreatComponent::UPREnemyThreatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}

void UPREnemyThreatComponent::AddThreat(AActor* Target, float Amount)
{
	// 타겟 선택은 서버에서만 결정한다. 클라이언트는 결과를 Blackboard/복제로 따라간다.
	if (!GetOwner() || !GetOwner()->HasAuthority() || !IsValid(Target) || Amount <= 0.0f)
	{
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();

	for (FPRThreatEntry& Entry : ThreatList)
	{
		if (Entry.Target == Target)
		{
			// 이미 추적 중인 대상이면 값만 누적하고 마지막 갱신 시간을 갱신한다.
			Entry.ThreatValue += Amount;
			Entry.LastUpdatedTime = CurrentTime;
			ReevaluateTarget();
			return;
		}
	}

	FPRThreatEntry& NewEntry = ThreatList.AddDefaulted_GetRef();
	NewEntry.Target = Target;
	NewEntry.ThreatValue = Amount;
	NewEntry.LastUpdatedTime = CurrentTime;

	ReevaluateTarget();
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

void UPREnemyThreatComponent::OnTargetLost(AActor* LostTarget)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !IsValid(LostTarget))
	{
		return;
	}

	for (int32 Index = ThreatList.Num() - 1; Index >= 0; --Index)
	{
		if (ThreatList[Index].Target == LostTarget)
		{
			ThreatList.RemoveAt(Index);
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

	AActor* BestTarget = nullptr;
	float BestThreat = 0.0f;
	float CurrentThreat = 0.0f;

	for (const FPRThreatEntry& Entry : ThreatList)
	{
		if (!IsValid(Entry.Target))
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
		SetCurrentTarget(BestTarget);
		return;
	}

	if (CurrentTarget == BestTarget)
	{
		return;
	}

	// 현재 타겟을 너무 쉽게 바꾸면 몬스터가 산만해지므로,
	// 위협 차이와 최소 전환 간격을 동시에 만족할 때만 교체한다.
	const float CurrentTime = GetWorld()->GetTimeSeconds();
	const bool bCanSwitchByRatio = BestThreat >= (CurrentThreat * SwitchRatio);
	const bool bCanSwitchByTime = (CurrentTime - LastSwitchTime) >= SwitchCooldown;

	if (bCanSwitchByRatio && bCanSwitchByTime)
	{
		SetCurrentTarget(BestTarget);
	}
}

void UPREnemyThreatComponent::SetCurrentTarget(AActor* NewTarget)
{
	if (CurrentTarget == NewTarget)
	{
		return;
	}

	AActor* OldTarget = CurrentTarget;
	CurrentTarget = NewTarget;
	LastSwitchTime = GetWorld()->GetTimeSeconds();

	OnTargetChanged.Broadcast(OldTarget, NewTarget);
}

void UPREnemyThreatComponent::CleanupInvalidEntries()
{
	const float CurrentTime = GetWorld()->GetTimeSeconds();

	for (int32 Index = ThreatList.Num() - 1; Index >= 0; --Index)
	{
		const bool bInvalidTarget = !IsValid(ThreatList[Index].Target);
		const bool bExpired = (CurrentTime - ThreatList[Index].LastUpdatedTime) > ThreatForgetTime;

		if (bInvalidTarget || bExpired)
		{
			ThreatList.RemoveAt(Index);
		}
	}
}

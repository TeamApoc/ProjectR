// Copyright ProjectR. All Rights Reserved.

#include "PRBossBaseCharacter.h"

#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/PRGameplayTags.h"

APRBossBaseCharacter::APRBossBaseCharacter()
{
	// 원본 보스 설계를 기준으로 한 페이즈 진입 체력 비율이다.
	PhaseThresholdRatios.Add(EPRFaerinPhase::AdvancedRanged, 0.87f);
	PhaseThresholdRatios.Add(EPRFaerinPhase::SwordJudgment, 0.65f);
	PhaseThresholdRatios.Add(EPRFaerinPhase::FinalTeleportLoop, 0.25f);
}

void APRBossBaseCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (!HasAuthority() || !IsValid(AbilitySystemComponent))
	{
		return;
	}

	if (TObjectPtr<UPRAbilitySet>* FoundAbilitySet = PhaseAbilitySets.Find(CurrentPhase))
	{
		if (IsValid(*FoundAbilitySet))
		{
			// 기본 Enemy AbilitySet 위에 현재 페이즈 전용 AbilitySet을 추가로 부여한다.
			AbilitySystemComponent->ClearAbilitySetByHandles(CurrentPhaseHandles);
			AbilitySystemComponent->GiveAbilitySet(*FoundAbilitySet, CurrentPhaseHandles);
		}
	}
}

void APRBossBaseCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APRBossBaseCharacter, CurrentPhase);
}

void APRBossBaseCharacter::OnHealthRatioChanged(float NewRatio)
{
	if (!HasAuthority())
	{
		return;
	}

	// 페이즈 전환 판단은 서버 체력 비율만 신뢰한다.
	if (CurrentPhase == EPRFaerinPhase::Opening && NewRatio <= PhaseThresholdRatios.FindRef(EPRFaerinPhase::AdvancedRanged))
	{
		BeginPhaseTransition(EPRFaerinPhase::AdvancedRanged);
	}
	else if (CurrentPhase == EPRFaerinPhase::AdvancedRanged && NewRatio <= PhaseThresholdRatios.FindRef(EPRFaerinPhase::SwordJudgment))
	{
		BeginPhaseTransition(EPRFaerinPhase::SwordJudgment);
	}
	else if (CurrentPhase == EPRFaerinPhase::SwordJudgment && NewRatio <= PhaseThresholdRatios.FindRef(EPRFaerinPhase::FinalTeleportLoop))
	{
		BeginPhaseTransition(EPRFaerinPhase::FinalTeleportLoop);
	}
}

void APRBossBaseCharacter::BeginPhaseTransition(EPRFaerinPhase NewPhase)
{
	if (!HasAuthority() || CurrentPhase == NewPhase || PendingPhase == NewPhase || !IsValid(AbilitySystemComponent))
	{
		return;
	}

	PendingPhase = NewPhase;
	AbilitySystemComponent->AddLooseGameplayTag(PRGameplayTags::State_PhaseTransitioning);

	// 페이즈 전환 Ability/연출이 붙기 전까지는 즉시 확정한다.
	// 이후 보스 전환 몽타주가 들어오면 해당 Ability가 CommitPhaseTransition을 호출하게 바꾸면 된다.
	CommitPhaseTransition(NewPhase);
}

void APRBossBaseCharacter::CommitPhaseTransition(EPRFaerinPhase NewPhase)
{
	if (!HasAuthority() || !IsValid(AbilitySystemComponent))
	{
		return;
	}

	const EPRFaerinPhase OldPhase = CurrentPhase;

	// 이전 페이즈에서만 쓰던 Ability/GE를 회수하고 새 페이즈 세트를 부여한다.
	AbilitySystemComponent->ClearAbilitySetByHandles(CurrentPhaseHandles);

	if (TObjectPtr<UPRAbilitySet>* FoundAbilitySet = PhaseAbilitySets.Find(NewPhase))
	{
		if (IsValid(*FoundAbilitySet))
		{
			AbilitySystemComponent->GiveAbilitySet(*FoundAbilitySet, CurrentPhaseHandles);
		}
	}

	CurrentPhase = NewPhase;
	PendingPhase = NewPhase;
	AbilitySystemComponent->RemoveLooseGameplayTag(PRGameplayTags::State_PhaseTransitioning);

	OnPhaseChanged.Broadcast(OldPhase, NewPhase);
}

void APRBossBaseCharacter::OnRep_CurrentPhase(EPRFaerinPhase OldPhase)
{
	// 클라이언트는 복제된 페이즈 변화로 연출/UI를 동기화한다.
	OnPhaseChanged.Broadcast(OldPhase, CurrentPhase);
}

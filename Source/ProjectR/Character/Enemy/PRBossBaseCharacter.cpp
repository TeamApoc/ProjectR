// Copyright ProjectR. All Rights Reserved.

#include "PRBossBaseCharacter.h"

#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/PRGameplayTags.h"

APRBossBaseCharacter::APRBossBaseCharacter()
{
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

	// 페이즈 전환 어빌리티가 붙기 전까지는 직접 커밋해서 흐름만 유지한다.
	CommitPhaseTransition(NewPhase);
}

void APRBossBaseCharacter::CommitPhaseTransition(EPRFaerinPhase NewPhase)
{
	if (!HasAuthority() || !IsValid(AbilitySystemComponent))
	{
		return;
	}

	const EPRFaerinPhase OldPhase = CurrentPhase;

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
	OnPhaseChanged.Broadcast(OldPhase, CurrentPhase);
}

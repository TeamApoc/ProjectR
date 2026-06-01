// Copyright ProjectR. All Rights Reserved.

#include "PRBossBaseCharacter.h"

#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/AI/Boss/PRBossPatternActor.h"
#include "ProjectR/PRGameplayTags.h"

APRBossBaseCharacter::APRBossBaseCharacter()
{
	// 페이즈 임계값은 보스 BP/데이터에서 설정한다. C++ 기본값을 두면 몬스터별 튜닝이 하드코딩된다.
	bUseWorldHealthBar = false;
}

void APRBossBaseCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CancelAllBossPatternActors();
	Super::EndPlay(EndPlayReason);
}

void APRBossBaseCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (!HasAuthority() || !IsValid(AbilitySystemComponent))
	{
		return;
	}

	if (!bUsePhaseAbilitySets)
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

void APRBossBaseCharacter::OnPostDamageApplied(const FPRDamageAppliedContext& Context)
{
	Super::OnPostDamageApplied(Context);

	if (Context.MaxHealth > 0.0f)
	{
		// 데미지 적용 직후 예측 체력 비율로 페이즈 전환을 검사한다.
		const float NewRatio = FMath::Clamp(Context.HealthAfterDamage / Context.MaxHealth, 0.0f, 1.0f);
		OnHealthRatioChanged(NewRatio);
	}
}

void APRBossBaseCharacter::OnHealthRatioChanged(float NewRatio)
{
	if (!HasAuthority())
	{
		return;
	}

	// 페이즈 전환 판단은 서버 체력 비율만 신뢰하고, enum 순서를 진행 순서로 사용한다.
	EPRBossPhase CandidatePhase = CurrentPhase;
	const uint8 CurrentPhaseIndex = static_cast<uint8>(CurrentPhase);
	uint8 CandidatePhaseIndex = CurrentPhaseIndex;

	for (const TPair<EPRBossPhase, float>& ThresholdPair : PhaseThresholdRatios)
	{
		const EPRBossPhase Phase = ThresholdPair.Key;
		const uint8 PhaseIndex = static_cast<uint8>(Phase);
		const float ThresholdRatio = ThresholdPair.Value;

		if (PhaseIndex <= CurrentPhaseIndex)
		{
			continue;
		}

		if (NewRatio <= ThresholdRatio && PhaseIndex > CandidatePhaseIndex)
		{
			CandidatePhase = Phase;
			CandidatePhaseIndex = PhaseIndex;
		}
	}

	if (CandidatePhase != CurrentPhase)
	{
		BeginPhaseTransition(CandidatePhase);
	}
}

void APRBossBaseCharacter::BeginPhaseTransition(EPRBossPhase NewPhase)
{
	if (!HasAuthority()
		|| CurrentPhase == NewPhase
		|| PendingPhase == NewPhase
		|| static_cast<uint8>(NewPhase) <= static_cast<uint8>(CurrentPhase)
		|| !IsValid(AbilitySystemComponent)
		|| AbilitySystemComponent->HasMatchingGameplayTag(PRGameplayTags::State_PhaseTransitioning))
	{
		return;
	}

	PendingPhase = NewPhase;
	AbilitySystemComponent->AddLooseGameplayTag(PRGameplayTags::State_PhaseTransitioning);
	AbilitySystemComponent->AddReplicatedLooseGameplayTag(PRGameplayTags::State_PhaseTransitioning);

	FGameplayTagContainer PatternCancelTags;
	PatternCancelTags.AddTag(PRGameplayTags::Ability_Boss_Pattern);
	AbilitySystemComponent->CancelAbilities(&PatternCancelTags);
	CancelBossPatternActorsForPhaseTransition();

	FGameplayEventData Payload;
	Payload.EventTag = PRGameplayTags::Event_Ability_PhaseTransition;
	Payload.Instigator = this;
	Payload.Target = this;
	Payload.EventMagnitude = static_cast<float>(static_cast<uint8>(NewPhase));

	// 페이즈 전환 Ability는 서버가 확정한 목표 페이즈를 GameplayEvent로 받아 시작한다.
	const int32 TriggeredAbilityCount = AbilitySystemComponent->HandleGameplayEvent(
		PRGameplayTags::Event_Ability_PhaseTransition,
		&Payload);

	if (TriggeredAbilityCount <= 0)
	{
		// 전환 Ability가 아직 연결되지 않은 테스트 상태에서는 페이즈가 고착되지 않도록 즉시 확정한다.
		CommitPhaseTransition(NewPhase);
	}
}

void APRBossBaseCharacter::CommitPhaseTransition(EPRBossPhase NewPhase)
{
	if (!HasAuthority() || !IsValid(AbilitySystemComponent))
	{
		return;
	}

	if (CurrentPhase == NewPhase)
	{
		PendingPhase = NewPhase;
		AbilitySystemComponent->RemoveLooseGameplayTag(PRGameplayTags::State_PhaseTransitioning);
		AbilitySystemComponent->RemoveReplicatedLooseGameplayTag(PRGameplayTags::State_PhaseTransitioning);
		return;
	}

	const EPRBossPhase OldPhase = CurrentPhase;

	if (bUsePhaseAbilitySets)
	{
		// 이전 페이즈에서만 열렸던 Ability/GE를 회수하고 새 페이즈 세트를 부여한다.
		AbilitySystemComponent->ClearAbilitySetByHandles(CurrentPhaseHandles);

		if (TObjectPtr<UPRAbilitySet>* FoundAbilitySet = PhaseAbilitySets.Find(NewPhase))
		{
			if (IsValid(*FoundAbilitySet))
			{
				AbilitySystemComponent->GiveAbilitySet(*FoundAbilitySet, CurrentPhaseHandles);
			}
		}
	}

	CurrentPhase = NewPhase;
	PendingPhase = NewPhase;
	AbilitySystemComponent->RemoveLooseGameplayTag(PRGameplayTags::State_PhaseTransitioning);
	AbilitySystemComponent->RemoveReplicatedLooseGameplayTag(PRGameplayTags::State_PhaseTransitioning);

	OnPhaseChanged.Broadcast(OldPhase, NewPhase);
}

void APRBossBaseCharacter::RegisterBossPatternActor(APRBossPatternActor* Actor)
{
	if (!HasAuthority() || !IsValid(Actor))
	{
		return;
	}

	ActivePatternActors.RemoveAll([](const APRBossPatternActor* RegisteredActor)
	{
		return !IsValid(RegisteredActor);
	});

	ActivePatternActors.AddUnique(Actor);
}

void APRBossBaseCharacter::UnregisterBossPatternActor(APRBossPatternActor* Actor)
{
	if (!HasAuthority())
	{
		return;
	}

	ActivePatternActors.RemoveAll([Actor](const APRBossPatternActor* RegisteredActor)
	{
		return !IsValid(RegisteredActor) || RegisteredActor == Actor;
	});
}

void APRBossBaseCharacter::CancelAllBossPatternActors()
{
	if (!HasAuthority())
	{
		return;
	}

	TArray<TObjectPtr<APRBossPatternActor>> PatternActorsToCancel = ActivePatternActors;
	ActivePatternActors.Reset();

	for (APRBossPatternActor* PatternActor : PatternActorsToCancel)
	{
		if (IsValid(PatternActor))
		{
			PatternActor->CancelPatternActor();
		}
	}
}

void APRBossBaseCharacter::CancelBossPatternActorsForPhaseTransition()
{
	if (!HasAuthority())
	{
		return;
	}

	TArray<TObjectPtr<APRBossPatternActor>> PatternActorsToCancel;
	TArray<TObjectPtr<APRBossPatternActor>> PatternActorsToKeep;
	for (APRBossPatternActor* PatternActor : ActivePatternActors)
	{
		if (!IsValid(PatternActor))
		{
			continue;
		}

		if (PatternActor->ShouldCancelOnBossPhaseTransition())
		{
			PatternActorsToCancel.Add(PatternActor);
		}
		else
		{
			PatternActorsToKeep.Add(PatternActor);
		}
	}

	ActivePatternActors = MoveTemp(PatternActorsToKeep);

	for (APRBossPatternActor* PatternActor : PatternActorsToCancel)
	{
		if (IsValid(PatternActor))
		{
			PatternActor->CancelPatternActor();
		}
	}
}

FText APRBossBaseCharacter::GetBossDisplayName() const
{
	return FText::FromName(CharacterID);
}

void APRBossBaseCharacter::GetPhaseThresholdRatios(TArray<float>& OutRatios) const
{
	OutRatios.Reset();

	for (const TPair<EPRBossPhase, float>& ThresholdPair : PhaseThresholdRatios)
	{
		OutRatios.Add(FMath::Clamp(ThresholdPair.Value, 0.0f, 1.0f));
	}

	OutRatios.Sort([](float Left, float Right)
	{
		return Left > Right;
	});
}

void APRBossBaseCharacter::HandleGameplayTagUpdated(const FGameplayTag& ChangedTag, bool TagExists)
{
	Super::HandleGameplayTagUpdated(ChangedTag, TagExists);

	if (!TagExists)
	{
		return;
	}

	if (ChangedTag.MatchesTag(PRGameplayTags::State_Dead)
		|| ChangedTag.MatchesTag(PRGameplayTags::State_Groggy))
	{
		CancelAllBossPatternActors();
	}
}

void APRBossBaseCharacter::OnRep_CurrentPhase(EPRBossPhase OldPhase)
{
	// 클라이언트는 복제된 페이즈 변화로 연출/UI를 동기화한다.
	OnPhaseChanged.Broadcast(OldPhase, CurrentPhase);
}

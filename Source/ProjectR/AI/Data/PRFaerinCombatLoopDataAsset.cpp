// Copyright ProjectR. All Rights Reserved.

#include "PRFaerinCombatLoopDataAsset.h"

#include "GameplayAbilitySpec.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/AI/Data/PRPatternDataAsset.h"

const FPRFaerinPhaseLoopConfig* UPRFaerinCombatLoopDataAsset::FindPhaseConfig(EPRBossPhase Phase) const
{
	for (const FPRFaerinPhaseLoopConfig& PhaseConfig : PhaseConfigs)
	{
		if (PhaseConfig.Phase == Phase)
		{
			return &PhaseConfig;
		}
	}

	return nullptr;
}

const FPRFaerinPatternLoopMetadata* UPRFaerinCombatLoopDataAsset::FindPatternMetadata(const FGameplayTag& AbilityTag) const
{
	if (!AbilityTag.IsValid())
	{
		return nullptr;
	}

	for (const FPRFaerinPatternLoopMetadata& Metadata : PatternMetadata)
	{
		if (Metadata.AbilityTag.MatchesTagExact(AbilityTag))
		{
			return &Metadata;
		}
	}

	return nullptr;
}

bool UPRFaerinCombatLoopDataAsset::ValidateLoopData(
	const UPRPatternDataAsset* PatternDataAsset,
	UPRAbilitySystemComponent* AbilitySystemComponent,
	TArray<FString>& OutErrors) const
{
	OutErrors.Reset();

	if (!IsValid(PatternDataAsset))
	{
		OutErrors.Add(TEXT("PatternDataAsset이 비어 있다."));
	}

	TSet<FGameplayTag> PatternRuleTags;
	if (IsValid(PatternDataAsset))
	{
		for (const FPRPatternRule& PatternRule : PatternDataAsset->PatternRules)
		{
			if (!PatternRule.IsValid())
			{
				continue;
			}

			PatternRuleTags.Add(PatternRule.AbilityTag);
			if (FindPatternMetadata(PatternRule.AbilityTag) == nullptr)
			{
				OutErrors.Add(FString::Printf(
					TEXT("PatternData Rule에 대응되는 Faerin Loop Metadata가 없다. AbilityTag=%s"),
					*PatternRule.AbilityTag.ToString()));
			}
		}
	}

	TSet<FGameplayTag> MetadataTags;
	for (const FPRFaerinPatternLoopMetadata& Metadata : PatternMetadata)
	{
		if (!Metadata.AbilityTag.IsValid())
		{
			OutErrors.Add(TEXT("Faerin Loop Metadata에 비어 있는 AbilityTag가 있다."));
			continue;
		}

		if (MetadataTags.Contains(Metadata.AbilityTag))
		{
			OutErrors.Add(FString::Printf(
				TEXT("Faerin Loop Metadata에 중복 AbilityTag가 있다. AbilityTag=%s"),
				*Metadata.AbilityTag.ToString()));
		}
		MetadataTags.Add(Metadata.AbilityTag);

		if (IsValid(PatternDataAsset) && !PatternRuleTags.Contains(Metadata.AbilityTag))
		{
			OutErrors.Add(FString::Printf(
				TEXT("Faerin Loop Metadata가 PatternData에 없는 AbilityTag를 참조한다. AbilityTag=%s"),
				*Metadata.AbilityTag.ToString()));
		}

		if (IsValid(AbilitySystemComponent))
		{
			FGameplayTagContainer QueryTags;
			QueryTags.AddTag(Metadata.AbilityTag);

			TArray<FGameplayAbilitySpec*> MatchingSpecs;
			AbilitySystemComponent->GetActivatableGameplayAbilitySpecsByAllMatchingTags(QueryTags, MatchingSpecs);
			if (MatchingSpecs.IsEmpty())
			{
				OutErrors.Add(FString::Printf(
					TEXT("Faerin AbilitySet에서 Loop Metadata AbilityTag로 활성화 가능한 GA를 찾지 못했다. AbilityTag=%s"),
					*Metadata.AbilityTag.ToString()));
			}
		}
	}

	for (const FPRFaerinPhaseLoopConfig& PhaseConfig : PhaseConfigs)
	{
		if (PhaseConfig.HighHealthRatioReference <= PhaseConfig.LowHealthRatioReference)
		{
			OutErrors.Add(FString::Printf(
				TEXT("Faerin PhaseConfig의 체력 비율 기준이 역전되어 있다. Phase=%s"),
				*StaticEnum<EPRBossPhase>()->GetNameStringByValue(static_cast<int64>(PhaseConfig.Phase))));
		}
	}

	return OutErrors.IsEmpty();
}

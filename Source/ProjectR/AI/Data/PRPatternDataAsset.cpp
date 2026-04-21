// Copyright ProjectR. All Rights Reserved.

#include "PRPatternDataAsset.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#if WITH_EDITOR
EDataValidationResult UPRPatternDataAsset::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	for (int32 RuleIndex = 0; RuleIndex < PatternRules.Num(); ++RuleIndex)
	{
		const FPRPatternRule& Rule = PatternRules[RuleIndex];
		if (!Rule.AbilityTag.IsValid())
		{
			Context.AddError(FText::FromString(
				FString::Printf(TEXT("PatternRules[%d]에는 유효한 AbilityTag가 필요합니다."), RuleIndex)));
			Result = EDataValidationResult::Invalid;
		}

		if (Rule.MinRange > Rule.MaxRange)
		{
			Context.AddError(FText::FromString(
				FString::Printf(TEXT("PatternRules[%d]의 MinRange는 MaxRange보다 클 수 없습니다."), RuleIndex)));
			Result = EDataValidationResult::Invalid;
		}

		if (Rule.SelectionWeight <= 0.0f)
		{
			Context.AddError(FText::FromString(
				FString::Printf(TEXT("PatternRules[%d]의 SelectionWeight는 0보다 커야 합니다."), RuleIndex)));
			Result = EDataValidationResult::Invalid;
		}
	}

	return Result;
}
#endif

// Copyright ProjectR. All Rights Reserved.

#include "PRMMC_AmmoCost.h"

#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Weapon.h"
#include "ProjectR/Combat/PRCombatGameplayTags.h"

namespace
{
	// Primary/Secondary AmmoScale을 모두 캡처한 정의를 한 번만 만들어 재사용
	struct FAmmoScaleCaptures
	{
		FGameplayEffectAttributeCaptureDefinition PrimaryAmmoScaleDef;
		FGameplayEffectAttributeCaptureDefinition SecondaryAmmoScaleDef;

		FAmmoScaleCaptures()
		{
			PrimaryAmmoScaleDef = FGameplayEffectAttributeCaptureDefinition(
				UPRAttributeSet_Weapon::GetPrimaryAmmoScaleAttribute(),
				EGameplayEffectAttributeCaptureSource::Source,
				false /* bSnapshot */);

			SecondaryAmmoScaleDef = FGameplayEffectAttributeCaptureDefinition(
				UPRAttributeSet_Weapon::GetSecondaryAmmoScaleAttribute(),
				EGameplayEffectAttributeCaptureSource::Source,
				false /* bSnapshot */);
		}
	};

	const FAmmoScaleCaptures& GetCaptures()
	{
		static FAmmoScaleCaptures Captures;
		return Captures;
	}
}

UPRMMC_AmmoCost::UPRMMC_AmmoCost()
{
	// 두 슬롯의 AmmoScale을 모두 캡처해 두고 TargetAmmoType에 따라 분기
	RelevantAttributesToCapture.Add(GetCaptures().PrimaryAmmoScaleDef);
	RelevantAttributesToCapture.Add(GetCaptures().SecondaryAmmoScaleDef);
}

float UPRMMC_AmmoCost::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvalParams;
	EvalParams.SourceTags = SourceTags;
	EvalParams.TargetTags = TargetTags;

	// 대상 슬롯에 해당하는 AmmoScale을 평가
	const FGameplayEffectAttributeCaptureDefinition& ScaleDef = (TargetAmmoType == EPRAmmoType::Primary)
		? GetCaptures().PrimaryAmmoScaleDef
		: GetCaptures().SecondaryAmmoScaleDef;

	float AmmoScale = 1.f;
	GetCapturedAttributeMagnitude(ScaleDef, Spec, EvalParams, AmmoScale);

	// SetByCaller로 전달된 displayed 발수. 미지정 시 단발 1.0 가정
	const float DisplayedCost = Spec.GetSetByCallerMagnitude(PRCombatGameplayTags::SetByCaller_AmmoCost, false, 1.0f);

	// raw 차감량 (양수). GE Modifier Op은 Subtract 또는 Coefficient -1로 사용
	return FMath::Max(DisplayedCost * AmmoScale, 0.f);
}

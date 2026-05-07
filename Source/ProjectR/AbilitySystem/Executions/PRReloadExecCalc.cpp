// Copyright ProjectR. All Rights Reserved.

#include "PRReloadExecCalc.h"

#include "AbilitySystemComponent.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Weapon.h"

struct FReloadCaptureDefs
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(PrimaryMagazineAmmo);
	DECLARE_ATTRIBUTE_CAPTUREDEF(PrimaryReserveAmmo);
	DECLARE_ATTRIBUTE_CAPTUREDEF(PrimaryAmmoScale);
	DECLARE_ATTRIBUTE_CAPTUREDEF(SecondaryMagazineAmmo);
	DECLARE_ATTRIBUTE_CAPTUREDEF(SecondaryReserveAmmo);
	DECLARE_ATTRIBUTE_CAPTUREDEF(SecondaryAmmoScale);

	FReloadCaptureDefs()
	{
		// Source 캡처. 재장전 GE는 ASC가 자기 자신에게 적용한다
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPRAttributeSet_Weapon, PrimaryMagazineAmmo, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPRAttributeSet_Weapon, PrimaryReserveAmmo, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPRAttributeSet_Weapon, PrimaryAmmoScale, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPRAttributeSet_Weapon, SecondaryMagazineAmmo, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPRAttributeSet_Weapon, SecondaryReserveAmmo, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPRAttributeSet_Weapon, SecondaryAmmoScale, Source, false);
	}
};

static const FReloadCaptureDefs& GetReloadCaptureDefs()
{
	static FReloadCaptureDefs Defs;
	return Defs;
}

UPRReloadExecCalc::UPRReloadExecCalc()
{
	const FReloadCaptureDefs& Defs = GetReloadCaptureDefs();
	RelevantAttributesToCapture.Add(Defs.PrimaryMagazineAmmoDef);
	RelevantAttributesToCapture.Add(Defs.PrimaryReserveAmmoDef);
	RelevantAttributesToCapture.Add(Defs.PrimaryAmmoScaleDef);
	RelevantAttributesToCapture.Add(Defs.SecondaryMagazineAmmoDef);
	RelevantAttributesToCapture.Add(Defs.SecondaryReserveAmmoDef);
	RelevantAttributesToCapture.Add(Defs.SecondaryAmmoScaleDef);
}

void UPRReloadExecCalc::Execute_Implementation(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FReloadCaptureDefs& Defs = GetReloadCaptureDefs();
	FAggregatorEvaluateParameters EvalParams;

	const bool bIsPrimary = (TargetAmmoType == EPRAmmoType::Primary);

	const FGameplayEffectAttributeCaptureDefinition& MagDef = bIsPrimary ? Defs.PrimaryMagazineAmmoDef : Defs.SecondaryMagazineAmmoDef;
	const FGameplayEffectAttributeCaptureDefinition& ReserveDef = bIsPrimary ? Defs.PrimaryReserveAmmoDef : Defs.SecondaryReserveAmmoDef;
	const FGameplayEffectAttributeCaptureDefinition& ScaleDef = bIsPrimary ? Defs.PrimaryAmmoScaleDef : Defs.SecondaryAmmoScaleDef;

	float Magazine = 0.f;
	float Reserve = 0.f;
	float Scale = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(MagDef, EvalParams, Magazine);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(ReserveDef, EvalParams, Reserve);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(ScaleDef, EvalParams, Scale);

	if (Scale <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	// 표시 발수 단위로 부족분과 가용분을 산출한 뒤 raw 단위로 환산
	const float NeededRawFloat = UPRAttributeSet_Weapon::MagazineRawBaseline - Magazine;
	const int32 NeededDisplayed = FMath::Max(FMath::FloorToInt(NeededRawFloat / Scale + KINDA_SMALL_NUMBER), 0);
	const int32 AvailableDisplayed = FMath::Max(FMath::FloorToInt(Reserve / Scale), 0);
	const int32 TransferDisplayed = FMath::Min(NeededDisplayed, AvailableDisplayed);
	if (TransferDisplayed <= 0)
	{
		return;
	}

	const float TransferRaw = TransferDisplayed * Scale;

	// 한 실행에서 두 어트리뷰트를 동시에 갱신: 탄창 += / 예비탄 -=
	const FGameplayAttribute MagAttr = bIsPrimary
		? UPRAttributeSet_Weapon::GetPrimaryMagazineAmmoAttribute()
		: UPRAttributeSet_Weapon::GetSecondaryMagazineAmmoAttribute();
	const FGameplayAttribute ReserveAttr = bIsPrimary
		? UPRAttributeSet_Weapon::GetPrimaryReserveAmmoAttribute()
		: UPRAttributeSet_Weapon::GetSecondaryReserveAmmoAttribute();

	OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
		MagAttr, EGameplayModOp::Additive, TransferRaw));
	OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
		ReserveAttr, EGameplayModOp::Additive, -TransferRaw));
}

// Copyright ProjectR. All Rights Reserved.

#include "PRDamageExecutionCalculation.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffectExtension.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Common.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Enemy.h"
#include "ProjectR/Character/Enemy/PRBossBaseCharacter.h"
#include "ProjectR/Combat/PRCombatStatics.h"
#include "ProjectR/Combat/PRCombatTypes.h"
#include "ProjectR/PRGameplayTags.h"

struct FPRDamageExecutionStatics
{
	// ExecutionCalculation은 직접 UPROPERTY를 들고 있지 못하므로 캡처 정의를 별도 구조체에 모아둔다.
	DECLARE_ATTRIBUTE_CAPTUREDEF(Health);
	DECLARE_ATTRIBUTE_CAPTUREDEF(MaxHealth);
	DECLARE_ATTRIBUTE_CAPTUREDEF(GroggyGauge);
	DECLARE_ATTRIBUTE_CAPTUREDEF(ArmorMultiplier);
	DECLARE_ATTRIBUTE_CAPTUREDEF(WeakpointMultiplier);

	FPRDamageExecutionStatics()
	{
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPRAttributeSet_Common, Health, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPRAttributeSet_Common, MaxHealth, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPRAttributeSet_Enemy, GroggyGauge, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPRAttributeSet_Enemy, ArmorMultiplier, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPRAttributeSet_Enemy, WeakpointMultiplier, Target, false);
	}
};

static const FPRDamageExecutionStatics& GetDamageExecutionStatics()
{
	static FPRDamageExecutionStatics Statics;
	return Statics;
}

UPRDamageExecutionCalculation::UPRDamageExecutionCalculation()
{
	RelevantAttributesToCapture.Add(GetDamageExecutionStatics().HealthDef);
	RelevantAttributesToCapture.Add(GetDamageExecutionStatics().MaxHealthDef);
	RelevantAttributesToCapture.Add(GetDamageExecutionStatics().GroggyGaugeDef);
	RelevantAttributesToCapture.Add(GetDamageExecutionStatics().ArmorMultiplierDef);
	RelevantAttributesToCapture.Add(GetDamageExecutionStatics().WeakpointMultiplierDef);
}

void UPRDamageExecutionCalculation::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();
	if (!IsValid(TargetASC))
	{
		return;
	}

	if (TargetASC->HasMatchingGameplayTag(PRGameplayTags::State_Dead)
		|| TargetASC->HasMatchingGameplayTag(PRGameplayTags::State_Invulnerable))
	{
		// 이미 죽었거나 무적인 대상은 피해/그로기 피해를 모두 무시한다.
		return;
	}

	const FGameplayEffectSpec& OwningSpec = ExecutionParams.GetOwningSpec();

	float BaseDamage = OwningSpec.GetSetByCallerMagnitude(PRCombatSetByCaller::Damage, false, 0.0f);
	float BaseGroggyDamage = OwningSpec.GetSetByCallerMagnitude(PRCombatSetByCaller::GroggyDamage, false, 0.0f);
	if (BaseDamage <= 0.0f && BaseGroggyDamage <= 0.0f)
	{
		return;
	}

	FAggregatorEvaluateParameters EvaluationParameters;

	float CurrentHealth = 0.0f;
	float MaxHealth = 0.0f;
	float CurrentGroggyGauge = 0.0f;
	float ArmorMultiplier = 1.0f;
	float WeakpointMultiplier = 1.0f;

	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(GetDamageExecutionStatics().HealthDef, EvaluationParameters, CurrentHealth);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(GetDamageExecutionStatics().MaxHealthDef, EvaluationParameters, MaxHealth);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(GetDamageExecutionStatics().GroggyGaugeDef, EvaluationParameters, CurrentGroggyGauge);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(GetDamageExecutionStatics().ArmorMultiplierDef, EvaluationParameters, ArmorMultiplier);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(GetDamageExecutionStatics().WeakpointMultiplierDef, EvaluationParameters, WeakpointMultiplier);

	// ApplyDamageEffect에서 Context에 넣은 HitResult를 다시 꺼내 Armor/Weakpoint 배율을 계산한다.
	const FHitResult EmptyHitResult;
	const FHitResult* HitResult = OwningSpec.GetContext().GetHitResult();
	const AActor* TargetActor = TargetASC->GetAvatarActor();
	const FPRDamageRegionInfo RegionInfo = UPRCombatStatics::ResolveDamageRegion(HitResult != nullptr ? *HitResult : EmptyHitResult, TargetActor);

	float RegionMultiplier = 1.0f;
	if (RegionInfo.IsWeakpoint())
	{
		RegionMultiplier = FMath::Max(WeakpointMultiplier, 0.0f);
	}
	else if (RegionInfo.IsArmor())
	{
		RegionMultiplier = FMath::Max(ArmorMultiplier, 0.0f);
	}

	const float FinalDamage = FMath::Max(BaseDamage * RegionMultiplier, 0.0f);
	const float FinalGroggyDamage = FMath::Max(BaseGroggyDamage * RegionMultiplier, 0.0f);

	if (FinalDamage > 0.0f)
	{
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
			UPRAttributeSet_Common::GetHealthAttribute(), EGameplayModOp::Additive, -FinalDamage));

		if (APRBossBaseCharacter* BossCharacter = Cast<APRBossBaseCharacter>(TargetASC->GetAvatarActor()))
		{
			if (MaxHealth > 0.0f)
			{
				// 보스는 피해 적용 직후의 체력 비율로 페이즈 전환을 검사한다.
				const float NewRatio = FMath::Clamp((CurrentHealth - FinalDamage) / MaxHealth, 0.0f, 1.0f);
				BossCharacter->OnHealthRatioChanged(NewRatio);
			}
		}
	}

	if (FinalGroggyDamage > 0.0f && CurrentGroggyGauge > 0.0f)
	{
		// GroggyGauge는 0에 가까워질수록 그로기에 가까운 구조다.
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
			UPRAttributeSet_Enemy::GetGroggyGaugeAttribute(), EGameplayModOp::Additive, -FinalGroggyDamage));
	}
}

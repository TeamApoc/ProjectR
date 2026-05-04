// Copyright ProjectR. All Rights Reserved.

#include "PRDamageExecCalc_FromPlayerMod.h"
#include "AbilitySystemComponent.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Common.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Enemy.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Player.h"
#include "ProjectR/Combat/PRCombatGameplayTags.h"
#include "ProjectR/Combat/PRCombatStatics.h"
#include "ProjectR/PRGameplayTags.h"

struct FFromPlayerModCaptureDefs
{
	// Target(피격 대상) 캡처 - Common AttributeSet
	DECLARE_ATTRIBUTE_CAPTUREDEF(Health);
	DECLARE_ATTRIBUTE_CAPTUREDEF(MaxHealth);
	DECLARE_ATTRIBUTE_CAPTUREDEF(Armor);

	// Target(피격 대상) 캡처 - Enemy AttributeSet
	DECLARE_ATTRIBUTE_CAPTUREDEF(GroggyGauge);

	// Source(공격자) 캡처 - Player AttributeSet
	DECLARE_ATTRIBUTE_CAPTUREDEF(CriticalHitChance);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CriticalDamageMultiplier);

	FFromPlayerModCaptureDefs()
	{
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPRAttributeSet_Common, Health, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPRAttributeSet_Common, MaxHealth, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPRAttributeSet_Common, Armor, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPRAttributeSet_Enemy, GroggyGauge, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPRAttributeSet_Player, CriticalHitChance, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPRAttributeSet_Player, CriticalDamageMultiplier, Source, false);
	}
};

static const FFromPlayerModCaptureDefs& GetFromPlayerModCaptureDefs()
{
	static FFromPlayerModCaptureDefs Defs;
	return Defs;
}

UPRDamageExecCalc_FromPlayerMod::UPRDamageExecCalc_FromPlayerMod()
{
	const FFromPlayerModCaptureDefs& Defs = GetFromPlayerModCaptureDefs();
	RelevantAttributesToCapture.Add(Defs.HealthDef);
	RelevantAttributesToCapture.Add(Defs.MaxHealthDef);
	RelevantAttributesToCapture.Add(Defs.ArmorDef);
	RelevantAttributesToCapture.Add(Defs.GroggyGaugeDef);
	RelevantAttributesToCapture.Add(Defs.CriticalHitChanceDef);
	RelevantAttributesToCapture.Add(Defs.CriticalDamageMultiplierDef);
}

void UPRDamageExecCalc_FromPlayerMod::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();
	UAbilitySystemComponent* SourceASC = ExecutionParams.GetSourceAbilitySystemComponent();
	if (!IsValid(TargetASC))
	{
		return;
	}

	// 사망/무적 상태 차단
	if (TargetASC->HasMatchingGameplayTag(PRGameplayTags::State_Dead)
		|| TargetASC->HasMatchingGameplayTag(PRGameplayTags::State_Invulnerable))
	{
		return;
	}

	const FGameplayEffectSpec& OwningSpec = ExecutionParams.GetOwningSpec();
	const FFromPlayerModCaptureDefs& Defs = GetFromPlayerModCaptureDefs();
	FAggregatorEvaluateParameters EvalParams;

	const FHitResult* HitResult = OwningSpec.GetContext().GetHitResult();
	const FHitResult EffectiveHitResult = HitResult != nullptr ? *HitResult : FHitResult();

	float CurrentHealth = 0.0f;
	float MaxHealth = 0.0f;
	float CurrentGroggyGauge = 0.0f;
	float TargetArmor = 0.0f;
	float CriticalHitChance = 0.0f;
	float CriticalDamageMultiplier = 1.0f;
	
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Defs.HealthDef, EvalParams, CurrentHealth);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Defs.MaxHealthDef, EvalParams, MaxHealth);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Defs.ArmorDef, EvalParams, TargetArmor);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Defs.GroggyGaugeDef, EvalParams, CurrentGroggyGauge);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Defs.CriticalHitChanceDef, EvalParams, CriticalHitChance);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Defs.CriticalDamageMultiplierDef, EvalParams, CriticalDamageMultiplier);
	
	AActor* SourceActor = IsValid(SourceASC) ? SourceASC->GetAvatarActor() : nullptr;
	AActor* TargetActor = TargetASC->GetAvatarActor();
	
	FPRDamageInputs Inputs;
	Inputs.bIsFromPlayer = true;
	Inputs.bIsFromFriendly = UPRCombatStatics::IsFriendly(SourceActor, TargetActor);
	Inputs.TargetArmor = TargetArmor;
	Inputs.CriticalHitChance = CriticalHitChance;
	Inputs.CriticalDamageMultiplier = CriticalDamageMultiplier;
	// 모드 스킬의 데미지와 그로기 데미지는 SetByCaller로 전달받는다
	Inputs.BaseDamage = OwningSpec.GetSetByCallerMagnitude(PRCombatGameplayTags::SetByCaller_Damage, false, 0.0f);
	Inputs.GroggyDamage = OwningSpec.GetSetByCallerMagnitude(PRCombatGameplayTags::SetByCaller_GroggyDamage, false, 0.0f);

	const FPRDamageOutputs Outputs = UPRCombatStatics::ComputeDamage(Inputs, EffectiveHitResult, TargetActor);

	/*~ Output Modifier 작성 ~*/
	if (Outputs.FinalDamage > 0.0f)
	{
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
			UPRAttributeSet_Common::GetIncomingDamageAttribute(), EGameplayModOp::Additive, Outputs.FinalDamage));
	}

	// 그로기 게이지는 적 대상에서만 출력한다
	if (Outputs.GroggyDamage > 0.0f && CurrentGroggyGauge > 0.0f)
	{
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
			UPRAttributeSet_Enemy::GetGroggyGaugeAttribute(), EGameplayModOp::Additive, -Outputs.GroggyDamage));
	}

	/*~ 수신자 후처리 ~*/
	DispatchPostDamageApplied(TargetActor, OwningSpec, Outputs, EffectiveHitResult, CurrentHealth, MaxHealth);
}

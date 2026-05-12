// Copyright ProjectR. All Rights Reserved.

#include "PRDamageExecCalc_FromEnemy.h"

#include "AbilitySystemComponent.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Common.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Enemy.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Player.h"
#include "ProjectR/Combat/PRCombatGameplayTags.h"
#include "ProjectR/Combat/PRCombatStatics.h"
#include "ProjectR/PRGameplayTags.h"

struct FFromEnemyCaptureDefs
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(Health);
	DECLARE_ATTRIBUTE_CAPTUREDEF(MaxHealth);
	DECLARE_ATTRIBUTE_CAPTUREDEF(Armor);
	DECLARE_ATTRIBUTE_CAPTUREDEF(AttackPower);

	FFromEnemyCaptureDefs()
	{
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPRAttributeSet_Common, Health, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPRAttributeSet_Common, MaxHealth, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPRAttributeSet_Common, Armor, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPRAttributeSet_Enemy, AttackPower, Source, false);
	}
};

static const FFromEnemyCaptureDefs& GetFromEnemyCaptureDefs()
{
	static FFromEnemyCaptureDefs Defs;
	return Defs;
}

UPRDamageExecCalc_FromEnemy::UPRDamageExecCalc_FromEnemy()
{
	const FFromEnemyCaptureDefs& Defs = GetFromEnemyCaptureDefs();
	RelevantAttributesToCapture.Add(Defs.HealthDef);
	RelevantAttributesToCapture.Add(Defs.MaxHealthDef);
	RelevantAttributesToCapture.Add(Defs.ArmorDef);
	RelevantAttributesToCapture.Add(Defs.AttackPowerDef);
}

void UPRDamageExecCalc_FromEnemy::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();
	UAbilitySystemComponent* SourceASC = ExecutionParams.GetSourceAbilitySystemComponent();
	if (!IsValid(TargetASC))
	{
		return;
	}

	if (TargetASC->HasMatchingGameplayTag(PRGameplayTags::State_Dead)
		|| TargetASC->HasMatchingGameplayTag(PRGameplayTags::State_Invulnerable))
	{
		return;
	}

	if (IsValid(TargetASC->GetSet<UPRAttributeSet_Enemy>()))
	{
		return;
	}

	const FGameplayEffectSpec& OwningSpec = ExecutionParams.GetOwningSpec();
	const FFromEnemyCaptureDefs& Defs = GetFromEnemyCaptureDefs();
	FAggregatorEvaluateParameters EvalParams;

	float CurrentHealth = 0.0f;
	float MaxHealth = 0.0f;
	float TargetArmor = 0.0f;
	float AttackPower = 0.0f;
	
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Defs.HealthDef, EvalParams, CurrentHealth);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Defs.MaxHealthDef, EvalParams, MaxHealth);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Defs.ArmorDef, EvalParams, TargetArmor);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Defs.AttackPowerDef, EvalParams, AttackPower);
	
	AActor* SourceActor = IsValid(SourceASC) ? SourceASC->GetAvatarActor() : nullptr;
	AActor* TargetActor = TargetASC->GetAvatarActor();
	
	FPRDamageInputs Inputs;
	Inputs.bIsFromPlayer = false;
	Inputs.bIsFromFriendly = UPRCombatStatics::IsFriendly(SourceActor, TargetActor);
	Inputs.TargetArmor = TargetArmor;
	
	const float DamageMultiplier = OwningSpec.GetSetByCallerMagnitude(PRCombatGameplayTags::SetByCaller_AttackMultiplier, false, 1.0f);
	if (AttackPower > 0.0f)
	{
		// 모든 적/보스 발신 공격은 StatRow AttackPower에 공격별 배율만 곱한다.
		Inputs.BaseDamage = AttackPower * DamageMultiplier;
	}

	const FHitResult EmptyHitResult;
	const FHitResult* HitResult = OwningSpec.GetContext().GetHitResult();
	const FHitResult& EffectiveHitResult = HitResult != nullptr ? *HitResult : EmptyHitResult;

	const FPRDamageOutputs Outputs = UPRCombatStatics::ComputeDamage(Inputs, EffectiveHitResult, TargetActor);

	if (Outputs.FinalDamage > 0.0f)
	{
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
			UPRAttributeSet_Common::GetIncomingDamageAttribute(), EGameplayModOp::Additive, Outputs.FinalDamage));

		if (IsValid(TargetASC->GetSet<UPRAttributeSet_Player>()))
		{
			OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
				UPRAttributeSet_Player::GetIncomingRecoverableDamageAttribute(), EGameplayModOp::Additive, Outputs.FinalDamage * 0.5f));
		}
	}

	const bool bHasSetByCallerGroggyDamageMultiplier = OwningSpec.SetByCallerTagMagnitudes.Contains(PRCombatGameplayTags::SetByCaller_GroggyDamageMultiplier);
	const UPRAttributeSet_Player* PlayerSet = TargetASC->GetSet<UPRAttributeSet_Player>();
	if (bHasSetByCallerGroggyDamageMultiplier && IsValid(PlayerSet) && Inputs.BaseDamage > 0.0f)
	{
		// 플레이어 강인도 피해도 기본 피해에서 파생한 그로기 피해에 공격별 배율을 곱한다.
		const float GroggyDamageMultiplier = OwningSpec.GetSetByCallerMagnitude(PRCombatGameplayTags::SetByCaller_GroggyDamageMultiplier, false, 1.0f);
		const float FinalPoiseDamage = UPRCombatStatics::CalculateBaseGroggyDamage(Inputs.BaseDamage) * FMath::Max(GroggyDamageMultiplier, 0.0f);
		if (FinalPoiseDamage > 0.0f)
		{
			OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
				UPRAttributeSet_Player::GetIncomingPoiseDamageAttribute(), EGameplayModOp::Additive, FinalPoiseDamage));
		}
	}

	DispatchPostDamageApplied(TargetActor, OwningSpec, Outputs, EffectiveHitResult, CurrentHealth, MaxHealth);
}

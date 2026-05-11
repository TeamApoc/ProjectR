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
	
	const float AttackMultiplier = OwningSpec.GetSetByCallerMagnitude(PRCombatGameplayTags::SetByCaller_AttackMultiplier, false, 1.0f);
	const bool bHasSetByCallerDamage = OwningSpec.SetByCallerTagMagnitudes.Contains(PRCombatGameplayTags::SetByCaller_Damage);
	const float RawDamage = OwningSpec.GetSetByCallerMagnitude(PRCombatGameplayTags::SetByCaller_Damage, false, 0.0f);
	if (bHasSetByCallerDamage)
	{
		// 적 패턴 DataAsset에서 전달된 피해가 있으면 Attribute AttackPower보다 우선한다.
		Inputs.BaseDamage = FMath::Max(RawDamage, 0.0f) * AttackMultiplier;
	}
	else if (AttackPower > 0.0f)
	{
		// 기존 적 공격은 AttackPower(Attribute) * AttackMultiplier(SetByCaller)를 유지한다.
		Inputs.BaseDamage = AttackPower * AttackMultiplier;
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

	const bool bHasSetByCallerGroggyDamage = OwningSpec.SetByCallerTagMagnitudes.Contains(PRCombatGameplayTags::SetByCaller_GroggyDamage);
	const UPRAttributeSet_Player* PlayerSet = TargetASC->GetSet<UPRAttributeSet_Player>();
	if (bHasSetByCallerGroggyDamage && IsValid(PlayerSet))
	{
		// 플레이어 강인도 피해는 방어력 경감 없이 DataAsset 수치와 공격 배수만 적용한다.
		const float RawGroggyDamage = OwningSpec.GetSetByCallerMagnitude(PRCombatGameplayTags::SetByCaller_GroggyDamage, false, 0.0f);
		const float FinalPoiseDamage = FMath::Max(RawGroggyDamage * AttackMultiplier, 0.0f);
		if (FinalPoiseDamage > 0.0f)
		{
			OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
				UPRAttributeSet_Player::GetIncomingPoiseDamageAttribute(), EGameplayModOp::Additive, FinalPoiseDamage));
		}
	}

	DispatchPostDamageApplied(TargetActor, OwningSpec, Outputs, EffectiveHitResult, CurrentHealth, MaxHealth);
}

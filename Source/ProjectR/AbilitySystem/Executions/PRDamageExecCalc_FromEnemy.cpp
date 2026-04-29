// Copyright ProjectR. All Rights Reserved.

#include "PRDamageExecCalc_FromEnemy.h"

#include "AbilitySystemComponent.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Common.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Enemy.h"
#include "ProjectR/Combat/PRCombatGameplayTags.h"
#include "ProjectR/Combat/PRCombatInterface.h"
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
	
	// 적의 공격력은 BaseDamage에 합성한다.
	// AttackPower(Attribute) * AttackMultiplier(SetByCaller)
	const float AttackMultiplier = OwningSpec.GetSetByCallerMagnitude(PRCombatGameplayTags::SetByCaller_AttackMultiplier, false, 1.0f);
	if (AttackPower > 0.0f)
	{
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
	}

	if (IPRCombatInterface* CombatTarget = Cast<IPRCombatInterface>(TargetActor))
	{
		FPRDamageAppliedContext Context;
		Context.FinalDamage = Outputs.FinalDamage;
		Context.FinalGroggyDamage = Outputs.GroggyDamage;
		Context.HealthBeforeDamage = CurrentHealth;
		Context.HealthAfterDamage = FMath::Clamp(CurrentHealth - Outputs.FinalDamage, 0.0f, MaxHealth);
		Context.MaxHealth = MaxHealth;
		Context.Region = Outputs.Region;
		Context.Instigator = OwningSpec.GetContext().GetOriginalInstigator();
		Context.HitResult = EffectiveHitResult;
		Context.bIsCritical = Outputs.bIsCritical;
		CombatTarget->OnPostDamageApplied(Context);
	}
}

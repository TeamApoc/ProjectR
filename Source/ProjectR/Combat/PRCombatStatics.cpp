// Copyright ProjectR. All Rights Reserved.

#include "PRCombatStatics.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "ProjectR/Combat/PRCombatInterface.h"

UAbilitySystemComponent* UPRCombatStatics::FindAbilitySystemComponent(const AActor* Actor)
{
	return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(const_cast<AActor*>(Actor));
}

EPRTeam UPRCombatStatics::GetActorTeam(const AActor* Actor)
{
	if (const IPRCombatInterface* Combat = Cast<IPRCombatInterface>(Actor))
	{
		return Combat->GetTeam();
	}
	return EPRTeam::Neutral;
}

bool UPRCombatStatics::IsFriendly(const AActor* SourceActor, const AActor* TargetActor)
{
	const EPRTeam SourceTeam = GetActorTeam(SourceActor);
	const EPRTeam TargetTeam = GetActorTeam(TargetActor);
	if (SourceTeam == EPRTeam::Neutral || TargetTeam == EPRTeam::Neutral)
	{
		return false;
	}
	return SourceTeam == TargetTeam;
}

FPRDamageOutputs UPRCombatStatics::ComputeDamage(const FPRDamageInputs& Inputs, const FHitResult& HitResult, const AActor* TargetActor)
{
	FPRDamageOutputs Outputs;
	Outputs.bIsFromFriendly = Inputs.bIsFromFriendly;
	
	// 부위 위치는 타깃이 알고 있다 (IPRCombatInterface::GetDamageRegionInfo)
	if (const IPRCombatInterface* Combat = Cast<IPRCombatInterface>(TargetActor))
	{
		Outputs.Region = Combat->GetDamageRegionInfo(HitResult.BoneName);
	}

	// 부위별 데미지 배율. 약점은 증폭, 장갑은 방어력 보너스로 처리
	float DamageMultiplier = 1.0f;
	float ArmorBonus = 0.0f;

	if (Outputs.Region.IsWeakpoint())
	{
		DamageMultiplier *= FMath::Max(Inputs.WeakpointMultiplier, 0.0f);
	}
	else if (Outputs.Region.IsArmor())
	{
		ArmorBonus = PRCombatConstants::ArmorRegionBonus;
	}
	
	// 프렌들리 파이어 감쇠
	if (Inputs.bIsFromFriendly)
	{
		if (Inputs.bIsFromPlayer)
		{
			DamageMultiplier *= FMath::Max(PRCombatConstants::FriendlyFireMultiplier_PvP, 0.f);
		}
		else
		{
			DamageMultiplier *= FMath::Max(PRCombatConstants::FriendlyFireMultiplier_EvE, 0.f);
		}
	}
	
	// 치명타 판정
	Outputs.bIsCritical = FMath::FRand() <= Inputs.CriticalHitChance;
	const float CriticalMultiplier = Outputs.bIsCritical ? FMath::Max(Inputs.CriticalDamageMultiplier, 1.0f) : 1.0f;

	// 배율 적용 (부위·프렌들리·치명타)
	const float PreArmorDamage = FMath::Max(Inputs.BaseDamage * DamageMultiplier * CriticalMultiplier, 0.0f);
	const float PreArmorGroggy = FMath::Max(Inputs.GroggyDamage * DamageMultiplier, 0.0f);

	// 방어력 경감: EffectiveArmor = max(Armor + ArmorBonus - ArmorPenetration, 0)
	// 경감률 = EffectiveArmor / (EffectiveArmor + K)
	const float EffectiveArmor = FMath::Max(Inputs.TargetArmor + ArmorBonus - Inputs.ArmorPenetration, 0.0f);
	const float ArmorReduction = EffectiveArmor / (EffectiveArmor + PRCombatConstants::ArmorScaling);

	Outputs.FinalDamage = PreArmorDamage * (1.0f - ArmorReduction);
	Outputs.GroggyDamage = PreArmorGroggy * (1.0f - ArmorReduction);

	return Outputs;
}

float UPRCombatStatics::CalculateBaseGroggyDamage(float BaseDamage)
{
	return BaseDamage * PRCombatConstants::GroggyDamageCoeff;
}
// Copyright ProjectR. All Rights Reserved.

#include "PRGameplayAbility_EnemyBase.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "ProjectR/AbilitySystem/Data/PRAbilitySystemRegistry.h"
#include "ProjectR/Character/Enemy/PREnemyBaseCharacter.h"
#include "ProjectR/Combat/PRCombatGameplayTags.h"
#include "ProjectR/System/PRAssetManager.h"

UPRGameplayAbility_EnemyBase::UPRGameplayAbility_EnemyBase()
{
	ActivationPolicy = EPRAbilityActivationPolicy::ServerInvoked;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

APREnemyBaseCharacter* UPRGameplayAbility_EnemyBase::GetEnemyAvatarCharacter() const
{
	return Cast<APREnemyBaseCharacter>(GetAvatarActorFromActorInfo());
}

void UPRGameplayAbility_EnemyBase::ApplyDamage(AActor* TargetActor, float AttackMultiplier, const FHitResult* HitResult)
{
	if (!IsValid(TargetActor))
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(SourceASC))
	{
		return;
	}

	const UPRAbilitySystemRegistry* Registry = UPRAssetManager::Get().GetAbilitySystemRegistry();
	if (!IsValid(Registry) || !IsValid(Registry->DamageGE_FromEnemy))
	{
		return;
	}

	const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(
		GetCurrentAbilitySpecHandle(),
		GetCurrentActorInfo(),
		GetCurrentActivationInfo(),
		Registry->DamageGE_FromEnemy);

	if (!SpecHandle.IsValid())
	{
		return;
	}

	// AttackMultiplier를 SetByCaller로 전달한다
	SpecHandle.Data->SetSetByCallerMagnitude(PRCombatGameplayTags::SetByCaller_AttackMultiplier, AttackMultiplier);

	// HitResult가 있으면 EffectContext에 포함시켜 ExecCalc에서 부위 판정에 활용한다
	if (HitResult != nullptr && HitResult->bBlockingHit)
	{
		SpecHandle.Data->GetContext().AddHitResult(*HitResult, true);
	}

	// 대상 ASC에 GE 적용
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	if (IsValid(TargetASC))
	{
		SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	}
}

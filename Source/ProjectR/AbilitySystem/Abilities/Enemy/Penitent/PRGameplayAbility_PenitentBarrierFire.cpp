// Copyright ProjectR. All Rights Reserved.

#include "PRGameplayAbility_PenitentBarrierFire.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "ProjectR/AbilitySystem/Data/PRAbilitySystemRegistry.h"
#include "PRPenitentBarrierActor.h"
#include "ProjectR/Character/Enemy/Penitent/PRPenitentCharacter.h"
#include "ProjectR/Combat/PRCombatGameplayTags.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/System/PRAssetManager.h"

UPRGameplayAbility_PenitentBarrierFire::UPRGameplayAbility_PenitentBarrierFire()
{
	FGameplayTagContainer BarrierFireTags;
	BarrierFireTags.AddTag(PRGameplayTags::Ability_Enemy_Pattern);
	BarrierFireTags.AddTag(PRGameplayTags::Ability_Enemy_Penitent_Pattern);
	BarrierFireTags.AddTag(PRGameplayTags::Ability_Enemy_Penitent_BarrierFire);
	SetAssetTags(BarrierFireTags);
}

bool UPRGameplayAbility_PenitentBarrierFire::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const UAbilitySystemComponent* AbilitySystemComponent = ActorInfo
		? ActorInfo->AbilitySystemComponent.Get()
		: nullptr;
	const APRPenitentCharacter* PenitentCharacter = ActorInfo
		? Cast<APRPenitentCharacter>(ActorInfo->AvatarActor.Get())
		: nullptr;

	return IsValid(AbilitySystemComponent)
		&& IsValid(PenitentCharacter)
		&& PenitentCharacter->HasActiveBarrier()
		&& AbilitySystemComponent->HasMatchingGameplayTag(PRGameplayTags::State_Enemy_Penitent_BarrierSummon);
}

void UPRGameplayAbility_PenitentBarrierFire::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	APRPenitentCharacter* PenitentCharacter = Cast<APRPenitentCharacter>(GetAvatarActorFromActorInfo());
	UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(PenitentCharacter)
		|| !PenitentCharacter->HasAuthority()
		|| !IsValid(AbilitySystemComponent)
		|| !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	APRPenitentBarrierActor* BarrierActor = PenitentCharacter->GetSpawnedBarrierActor();
	if (!IsValid(BarrierActor))
	{
		if (AbilitySystemComponent->HasMatchingGameplayTag(PRGameplayTags::State_Enemy_Penitent_BarrierSummon))
		{
			// 무효 배리어 상태 태그 정리
			AbilitySystemComponent->RemoveLooseGameplayTag(PRGameplayTags::State_Enemy_Penitent_BarrierSummon);
		}
		PenitentCharacter->ClearSpawnedBarrierActor();
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const FGameplayEffectSpecHandle DamageEffectSpecHandle = BuildBarrierDamageEffectSpec();
	if (DamageEffectSpecHandle.IsValid())
	{
		BarrierActor->SetBarrierDamageEffectSpec(DamageEffectSpecHandle);
	}

	BarrierActor->FireBarrier(PenitentCharacter->GetActorForwardVector());
	if (AbilitySystemComponent->HasMatchingGameplayTag(PRGameplayTags::State_Enemy_Penitent_BarrierSummon))
	{
		// 배리어 보유 상태 태그 정리
		AbilitySystemComponent->RemoveLooseGameplayTag(PRGameplayTags::State_Enemy_Penitent_BarrierSummon);
	}
	PenitentCharacter->ClearSpawnedBarrierActor(BarrierActor);
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

FGameplayEffectSpecHandle UPRGameplayAbility_PenitentBarrierFire::BuildBarrierDamageEffectSpec() const
{
	UAbilitySystemComponent* SourceAbilitySystemComponent = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(SourceAbilitySystemComponent))
	{
		return FGameplayEffectSpecHandle();
	}

	TSubclassOf<UGameplayEffect> DamageEffectClass = BarrierDamageEffectClass;
	if (!IsValid(DamageEffectClass))
	{
		const UPRAbilitySystemRegistry* Registry = UPRAssetManager::Get().GetAbilitySystemRegistry();
		if (IsValid(Registry))
		{
			DamageEffectClass = Registry->DamageGE_FromEnemy;
		}
	}

	if (!IsValid(DamageEffectClass))
	{
		return FGameplayEffectSpecHandle();
	}

	FGameplayEffectContextHandle EffectContext = SourceAbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(GetAvatarActorFromActorInfo());

	FGameplayEffectSpecHandle SpecHandle = SourceAbilitySystemComponent->MakeOutgoingSpec(
		DamageEffectClass,
		GetAbilityLevel(),
		EffectContext);

	if (SpecHandle.IsValid())
	{
		SpecHandle.Data->SetSetByCallerMagnitude(
			PRCombatGameplayTags::SetByCaller_AttackMultiplier,
			FMath::Max(DamageMultiplier, 0.0f));

		SpecHandle.Data->SetSetByCallerMagnitude(
			PRCombatGameplayTags::SetByCaller_PoiseDamage,
			FMath::Max(PoiseDamage, 0.0f));
	}

	return SpecHandle;
}

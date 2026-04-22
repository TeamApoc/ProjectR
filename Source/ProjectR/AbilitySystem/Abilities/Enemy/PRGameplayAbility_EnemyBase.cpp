// Copyright ProjectR. All Rights Reserved.

#include "PRGameplayAbility_EnemyBase.h"

#include "GameplayEffect.h"
#include "ProjectR/AbilitySystem/Effects/PRGameplayEffect_Damage.h"
#include "ProjectR/Character/Enemy/PREnemyBaseCharacter.h"
#include "ProjectR/Combat/PRCombatStatics.h"

UPRGameplayAbility_EnemyBase::UPRGameplayAbility_EnemyBase()
{
	ActivationPolicy = EPRAbilityActivationPolicy::ServerInvoked;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	DamageEffectClass = UPRGameplayEffect_Damage::StaticClass();
}

bool UPRGameplayAbility_EnemyBase::ApplyDamageContext(FPRDamageContext DamageContext) const
{
	if (!IsValid(DamageContext.SourceActor))
	{
		DamageContext.SourceActor = GetAvatarActorFromActorInfo();
	}

	if (!IsValid(DamageContext.EffectCauser))
	{
		DamageContext.EffectCauser = DamageContext.SourceActor;
	}

	if (!IsValid(DamageContext.SourceObject))
	{
		DamageContext.SourceObject = const_cast<UPRGameplayAbility_EnemyBase*>(this);
	}

	return UPRCombatStatics::ApplyDamageEffect(DamageContext, DamageEffectClass);
}

APREnemyBaseCharacter* UPRGameplayAbility_EnemyBase::GetEnemyAvatarCharacter() const
{
	return Cast<APREnemyBaseCharacter>(GetAvatarActorFromActorInfo());
}

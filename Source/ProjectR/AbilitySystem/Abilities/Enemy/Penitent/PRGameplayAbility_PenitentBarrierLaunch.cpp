// Copyright ProjectR. All Rights Reserved.

#include "PRGameplayAbility_PenitentBarrierLaunch.h"

#include "AbilitySystemComponent.h"
#include "ProjectR/Projectile/PRGroundBoxProjectileBase.h"
#include "ProjectR/Character/Enemy/Penitent/PRPenitentCharacter.h"
#include "ProjectR/PRGameplayTags.h"

UPRGameplayAbility_PenitentBarrierLaunch::UPRGameplayAbility_PenitentBarrierLaunch()
{
	FGameplayTagContainer BarrierLaunchTags;
	BarrierLaunchTags.AddTag(PRGameplayTags::Ability_Enemy_Pattern);
	BarrierLaunchTags.AddTag(PRGameplayTags::Ability_Enemy_Penitent_Pattern);
	BarrierLaunchTags.AddTag(PRGameplayTags::Ability_Enemy_Penitent_BarrierFire);
	SetAssetTags(BarrierLaunchTags);
}

bool UPRGameplayAbility_PenitentBarrierLaunch::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
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

void UPRGameplayAbility_PenitentBarrierLaunch::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
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

	APRGroundBoxProjectileBase* BarrierActor = PenitentCharacter->GetSpawnedBarrierActor();
	if (!IsValid(BarrierActor))
	{
		// 무효 배리어 상태 정리
		PenitentCharacter->CleanupSummonedBarrier(false);
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 부착 해제 후 런치 전환
	BarrierActor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	BarrierActor->LaunchGroundBoxProjectile(PenitentCharacter->GetActorForwardVector(), LaunchSpeed);

	// 런치 배리어 소유 해제
	PenitentCharacter->CleanupSummonedBarrier(false);
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

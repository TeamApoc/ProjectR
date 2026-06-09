// Copyright ProjectR. All Rights Reserved.

#include "PRGameplayAbility_FaeRoyalArcherWakeFromPerch.h"

#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/Character/Enemy/FaeRoyalArcher/PRFaeRoyalArcherCharacter.h"

UPRGameplayAbility_FaeRoyalArcherWakeFromPerch::UPRGameplayAbility_FaeRoyalArcherWakeFromPerch()
{
	FGameplayTagContainer WakeFromPerchTags;
	WakeFromPerchTags.AddTag(PRGameplayTags::Ability_Enemy_Pattern);
	WakeFromPerchTags.AddTag(PRGameplayTags::Ability_Enemy_RoyalArcher_Pattern);
	WakeFromPerchTags.AddTag(PRGameplayTags::Ability_Enemy_RoyalArcher_WakeFromPerch);
	SetAssetTags(WakeFromPerchTags);

	MontagePlayRate = 1.0f;
	bEndWhenMontageEnds = true;
	AlertDuration = 1.0f;
	bStopMovementOnAlert = true;
}

void UPRGameplayAbility_FaeRoyalArcherWakeFromPerch::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	APRFaeRoyalArcherCharacter* RoyalArcher = Cast<APRFaeRoyalArcherCharacter>(GetAvatarActorFromActorInfo());
	if (IsValid(RoyalArcher))
	{
		RoyalArcher->PrepareWakeFromPerch();
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

// Copyright ProjectR. All Rights Reserved.

#include "PRGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "ProjectR/PRGameplayTags.h"

// =====  UGameplayAbility Interface =====

bool UPRGameplayAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                             const FGameplayAbilityActorInfo* ActorInfo,
                                             const FGameplayTagContainer* SourceTags,
                                             const FGameplayTagContainer* TargetTags,
                                             FGameplayTagContainer* OptionalRelevantTags) const
{
	// GAS 표준 검사 후 실패 사유 분류. OptionalRelevantTags에 Fail.* 누적
	const bool bCanActivate = Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
	if (bCanActivate)
	{
		return true;
	}

	if (OptionalRelevantTags == nullptr || ActorInfo == nullptr)
	{
		return false;
	}

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!IsValid(ASC))
	{
		OptionalRelevantTags->AddTag(PRGameplayTags::Fail_Invalid);
		return false;
	}

	// Cooldown
	if (!CheckCooldown(Handle, ActorInfo, nullptr))
	{
		OptionalRelevantTags->AddTag(PRGameplayTags::Fail_Cooldown);
	}
	// Cost
	if (!CheckCost(Handle, ActorInfo, nullptr))
	{
		OptionalRelevantTags->AddTag(PRGameplayTags::Fail_Cost);
	}
	// Block 태그
	FGameplayTagContainer OwnedTags;
	ASC->GetOwnedGameplayTags(OwnedTags);
	if (ActivationBlockedTags.Num() > 0 && OwnedTags.HasAny(ActivationBlockedTags))
	{
		OptionalRelevantTags->AddTag(PRGameplayTags::Fail_Blocked);
	}

	// 위 3종 중 하나도 매칭 안 됐으면 Invalid 처리
	const bool bAnyReason =
		OptionalRelevantTags->HasTag(PRGameplayTags::Fail_Cooldown) ||
		OptionalRelevantTags->HasTag(PRGameplayTags::Fail_Cost) ||
		OptionalRelevantTags->HasTag(PRGameplayTags::Fail_Blocked);
	if (!bAnyReason)
	{
		OptionalRelevantTags->AddTag(PRGameplayTags::Fail_Invalid);
	}

	return false;
}

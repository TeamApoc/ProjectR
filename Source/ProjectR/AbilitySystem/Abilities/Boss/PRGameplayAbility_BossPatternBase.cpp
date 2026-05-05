// Copyright ProjectR. All Rights Reserved.

#include "PRGameplayAbility_BossPatternBase.h"

#include "AbilitySystemComponent.h"
#include "ProjectR/AbilitySystem/Abilities/Boss/PRBossAbilityTagUtils.h"
#include "ProjectR/AI/Components/PREnemyThreatComponent.h"
#include "ProjectR/Character/Enemy/PRBossBaseCharacter.h"
#include "ProjectR/PRGameplayTags.h"

UPRGameplayAbility_BossPatternBase::UPRGameplayAbility_BossPatternBase()
{
	SetAssetTags(PRBossAbility::MakePatternAssetTags(FGameplayTag()));

	ActivationBlockedTags.AddTag(PRGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(PRGameplayTags::State_Groggy);
	ActivationBlockedTags.AddTag(PRGameplayTags::State_PhaseTransitioning);
}

APRBossBaseCharacter* UPRGameplayAbility_BossPatternBase::GetBossAvatarCharacter() const
{
	return Cast<APRBossBaseCharacter>(GetAvatarActorFromActorInfo());
}

bool UPRGameplayAbility_BossPatternBase::CanRunBossPattern() const
{
	const APRBossBaseCharacter* BossCharacter = GetBossAvatarCharacter();
	const UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponentFromActorInfo();

	return IsValid(BossCharacter)
		&& IsValid(AbilitySystemComponent)
		&& !AbilitySystemComponent->HasMatchingGameplayTag(PRGameplayTags::State_Dead)
		&& !AbilitySystemComponent->HasMatchingGameplayTag(PRGameplayTags::State_Groggy)
		&& !AbilitySystemComponent->HasMatchingGameplayTag(PRGameplayTags::State_PhaseTransitioning);
}

AActor* UPRGameplayAbility_BossPatternBase::GetBossPatternTarget() const
{
	const APRBossBaseCharacter* BossCharacter = GetBossAvatarCharacter();
	if (!IsValid(BossCharacter))
	{
		return nullptr;
	}

	const UPREnemyThreatComponent* ThreatComponent = BossCharacter->GetEnemyThreatComponent();
	return IsValid(ThreatComponent) ? ThreatComponent->GetCurrentTarget() : nullptr;
}

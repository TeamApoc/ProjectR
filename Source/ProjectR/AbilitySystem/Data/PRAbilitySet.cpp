#include "PRAbilitySet.h"

#include "ProjectR/AbilitySystem/PRGameplayAbility.h"

bool FPRAbilityEntry::IsValid() const
{
	return ::IsValid(AbilityClass);
}

bool FPREffectEntry::IsValid() const
{
	return ::IsValid(EffectClass);
}

UPRAbilitySet::UPRAbilitySet()
{
	
}

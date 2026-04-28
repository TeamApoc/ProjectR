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

UPRAbilitySet* CreateRuntimeAbilitySet(
	UObject* Outer,
	const TArray<FPRAbilityEntry>& InAbilities,
	const TArray<FPREffectEntry>& InEffects)
{
	if (!IsValid(Outer))
	{
		return nullptr;
	}

	UPRAbilitySet* RuntimeAbilitySet = NewObject<UPRAbilitySet>(Outer);
	RuntimeAbilitySet->Abilities = InAbilities;
	RuntimeAbilitySet->Effects = InEffects;

	return RuntimeAbilitySet;
}

UPRAbilitySet::UPRAbilitySet()
{
}

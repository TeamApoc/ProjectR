// Copyright ProjectR. All Rights Reserved.

#include "PRAnimNotify_EnemyMeleeHit.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameplayAbilitySpec.h"
#include "ProjectR/AbilitySystem/Abilities/Enemy/PRGameplayAbility_EnemyMeleeAttack.h"

FString UPRAnimNotify_EnemyMeleeHit::GetNotifyName_Implementation() const
{
	return TEXT("PR Enemy Melee Hit");
}

void UPRAnimNotify_EnemyMeleeHit::Notify(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	if (!IsValid(MeshComp))
	{
		return;
	}

	AActor* OwnerActor = MeshComp->GetOwner();
	if (!IsValid(OwnerActor) || !OwnerActor->HasAuthority())
	{
		return;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerActor);
	if (!IsValid(ASC))
	{
		return;
	}

	for (const FGameplayAbilitySpec& AbilitySpec : ASC->GetActivatableAbilities())
	{
		if (!AbilitySpec.IsActive())
		{
			continue;
		}

		const TArray<UGameplayAbility*> AbilityInstances = AbilitySpec.GetAbilityInstances();
		for (UGameplayAbility* AbilityInstance : AbilityInstances)
		{
			UPRGameplayAbility_EnemyMeleeAttack* MeleeAbility = Cast<UPRGameplayAbility_EnemyMeleeAttack>(AbilityInstance);
			if (!IsValid(MeleeAbility))
			{
				continue;
			}

			MeleeAbility->TriggerMeleeHitFromAnimation();
			return;
		}
	}
}

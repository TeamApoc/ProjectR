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
		// 데미지 판정은 서버의 Ability 인스턴스만 실행한다.
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
			// 현재 재생 중인 공격 Ability를 찾아 Notify 프레임에 맞춰 한 번만 타격시킨다.
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

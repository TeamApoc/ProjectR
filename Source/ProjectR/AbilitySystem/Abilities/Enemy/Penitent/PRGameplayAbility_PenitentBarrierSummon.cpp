// Copyright ProjectR. All Rights Reserved.

#include "PRGameplayAbility_PenitentBarrierSummon.h"

#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "ProjectR/Actors/Enemy/PRPenitentBarrierActor.h"
#include "ProjectR/Character/Enemy/Penitent/PRPenitentCharacter.h"
#include "ProjectR/PRGameplayTags.h"

UPRGameplayAbility_PenitentBarrierSummon::UPRGameplayAbility_PenitentBarrierSummon()
{
	FGameplayTagContainer BarrierSummonTags;
	BarrierSummonTags.AddTag(PRGameplayTags::Ability_Enemy_Pattern);
	BarrierSummonTags.AddTag(PRGameplayTags::Ability_Enemy_Penitent_Pattern);
	BarrierSummonTags.AddTag(PRGameplayTags::Ability_Enemy_Penitent_BarrierSummon);
	SetAssetTags(BarrierSummonTags);
	ActivationBlockedTags.AddTag(PRGameplayTags::State_Enemy_Penitent_BarrierSummon);
}

bool UPRGameplayAbility_PenitentBarrierSummon::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
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
	return IsValid(AbilitySystemComponent)
		&& IsValid(BarrierActorClass)
		&& !AbilitySystemComponent->HasMatchingGameplayTag(PRGameplayTags::State_Enemy_Penitent_BarrierSummon);
}

void UPRGameplayAbility_PenitentBarrierSummon::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	APRPenitentCharacter* PenitentCharacter = Cast<APRPenitentCharacter>(GetAvatarActorFromActorInfo());
	UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponentFromActorInfo();
	UWorld* World = GetWorld();
	if (!IsValid(PenitentCharacter)
		|| !PenitentCharacter->HasAuthority()
		|| !IsValid(AbilitySystemComponent)
		|| !IsValid(World)
		|| !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (PenitentCharacter->HasActiveBarrier()
		|| AbilitySystemComponent->HasMatchingGameplayTag(PRGameplayTags::State_Enemy_Penitent_BarrierSummon))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const USkeletalMeshComponent* MeshComponent = PenitentCharacter->GetMesh();
	const bool bHasAttachSocket = IsValid(MeshComponent)
		&& BarrierAttachSocketName != NAME_None
		&& MeshComponent->DoesSocketExist(BarrierAttachSocketName);
	const FTransform SpawnTransform = bHasAttachSocket
		? MeshComponent->GetSocketTransform(BarrierAttachSocketName)
		: FTransform(PenitentCharacter->GetActorRotation(), PenitentCharacter->GetActorTransform().TransformPositionNoScale(BarrierSpawnOffset));

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = PenitentCharacter;
	SpawnParameters.Instigator = PenitentCharacter;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	APRPenitentBarrierActor* BarrierActor = World->SpawnActor<APRPenitentBarrierActor>(
		BarrierActorClass,
		SpawnTransform,
		SpawnParameters);

	if (!IsValid(BarrierActor))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (bHasAttachSocket)
	{
		// 소켓 부착
		BarrierActor->AttachToComponent(PenitentCharacter->GetMesh(), FAttachmentTransformRules::KeepWorldTransform, BarrierAttachSocketName);
	}
	else
	{
		// 액터 부착
		BarrierActor->AttachToActor(PenitentCharacter, FAttachmentTransformRules::KeepWorldTransform);
	}

	BarrierActor->InitializeAttachedBarrier(PenitentCharacter);
	PenitentCharacter->SetSpawnedBarrierActor(BarrierActor);
	AbilitySystemComponent->AddLooseGameplayTag(PRGameplayTags::State_Enemy_Penitent_BarrierSummon);
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

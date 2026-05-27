// Copyright ProjectR. All Rights Reserved.

#include "PRGameplayAbility_PenitentBarrierSummon.h"

#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "ProjectR/AbilitySystem/Abilities/Enemy/Penitent/PRPenitentBarrierActor.h"
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
	// 유효성 체크, 서버 실행 확인
	if (!IsValid(PenitentCharacter)
		|| !PenitentCharacter->HasAuthority()
		|| !IsValid(AbilitySystemComponent)
		|| !IsValid(World)
		|| !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 이미 배리어를 소환했다면 종료
	if (PenitentCharacter->HasActiveBarrier()
		|| AbilitySystemComponent->HasMatchingGameplayTag(PRGameplayTags::State_Enemy_Penitent_BarrierSummon))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	//
	const USkeletalMeshComponent* MeshComponent = PenitentCharacter->GetMesh();
	USceneComponent* BarrierAttachPoint = PenitentCharacter->GetBarrierAttachPoint();
	const bool bHasAttachPoint = IsValid(BarrierAttachPoint);
	const FTransform SpawnTransform = bHasAttachPoint
		? BarrierAttachPoint->GetComponentTransform()
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

	if (bHasAttachPoint)
	{
		// 소켓 부착
		BarrierActor->AttachToComponent(BarrierAttachPoint, FAttachmentTransformRules::KeepWorldTransform);
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

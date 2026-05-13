// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRGA_SwapWeapon.h"

#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/Weapon/Components/PRWeaponManagerComponent.h"
#include "ProjectR/Weapon/Items/PRItemInstance_Weapon.h"

UPRGA_SwapWeapon::UPRGA_SwapWeapon()
{
	AbilityTags.AddTag(PRGameplayTags::Ability_Player_SwapWeapon);
	InputTag = PRGameplayTags::Input_Ability_SwapWeapon;

	// 발사·재장전·사격 모드 전환 중 슬롯 교체로 인한 상태 꼬임 방지
	ActivationBlockedTags.AddTag(PRGameplayTags::State_Reloading);
	ActivationBlockedTags.AddTag(PRGameplayTags::State_Dodging);

	// SetCurrentWeaponSlot이 내부에서 클라→서버 RPC를 처리하므로 어빌리티는 서버에서만 실행
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	ActivationPolicy = EPRAbilityActivationPolicy::OnInputTriggered;
}

bool UPRGA_SwapWeapon::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const APRPlayerCharacter* PlayerCharacter = ActorInfo != nullptr
		? Cast<APRPlayerCharacter>(ActorInfo->AvatarActor.Get())
		: nullptr;
	if (!IsValid(PlayerCharacter))
	{
		if (OptionalRelevantTags != nullptr)
		{
			OptionalRelevantTags->AddTag(PRGameplayTags::Fail_Invalid);
		}
		return false;
	}

	const UPRWeaponManagerComponent* WeaponManager = PlayerCharacter->GetWeaponManager();
	if (!IsValid(WeaponManager))
	{
		if (OptionalRelevantTags != nullptr)
		{
			OptionalRelevantTags->AddTag(PRGameplayTags::Fail_Invalid);
		}
		return false;
	}

	// 현재 활성 슬롯의 반대 슬롯에 무기가 있어야 교체 가능
	const EPRWeaponSlotType CurrentSlot = WeaponManager->GetCurrentWeaponSlot();
	const EPRWeaponSlotType TargetSlot = (CurrentSlot == EPRWeaponSlotType::Primary)
		? EPRWeaponSlotType::Secondary
		: EPRWeaponSlotType::Primary;

	if (!IsValid(WeaponManager->GetWeaponInstanceBySlotType(TargetSlot)))
	{
		if (OptionalRelevantTags != nullptr)
		{
			OptionalRelevantTags->AddTag(PRGameplayTags::Fail_Invalid);
		}
		return false;
	}

	return true;
}

void UPRGA_SwapWeapon::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	APRPlayerCharacter* PlayerCharacter = Cast<APRPlayerCharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(PlayerCharacter))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UPRWeaponManagerComponent* WeaponManager = PlayerCharacter->GetWeaponManager();
	if (!IsValid(WeaponManager))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 활성 슬롯을 반대 슬롯으로 토글. SetCurrentWeaponSlot이 서버 권위 처리와 RPC 전달을 모두 담당
	const EPRWeaponSlotType CurrentSlot = WeaponManager->GetCurrentWeaponSlot();
	const EPRWeaponSlotType TargetSlot = (CurrentSlot == EPRWeaponSlotType::Primary)
		? EPRWeaponSlotType::Secondary
		: EPRWeaponSlotType::Primary;

	WeaponManager->SetCurrentWeaponSlot(TargetSlot);

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

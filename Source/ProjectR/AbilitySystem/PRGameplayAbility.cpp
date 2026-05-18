// Copyright ProjectR. All Rights Reserved.

#include "PRGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Data/PRAbilitySystemRegistry.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/Combat/PRCombatGameplayTags.h"
#include "ProjectR/System/PRAssetManager.h"
#include "ProjectR/Weapon/Components/PRWeaponManagerComponent.h"
#include "ProjectR/Weapon/Types/PRWeaponTypes.h"

// =====  UGameplayAbility Interface =====

bool UPRGameplayAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                             const FGameplayAbilityActorInfo* ActorInfo,
                                             const FGameplayTagContainer* SourceTags,
                                             const FGameplayTagContainer* TargetTags,
                                             FGameplayTagContainer* OptionalRelevantTags) const
{
	// GAS 표준 검사 후 실패 사유 분류. OptionalRelevantTags에 Fail.* 누적
	const bool bCanActivate = Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
	if (!bCanActivate && OptionalRelevantTags && ActorInfo)
	{
		
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
	}
	
	return bCanActivate;
}

UPRWeaponManagerComponent* UPRGameplayAbility::GetWeaponManager(const FGameplayAbilityActorInfo* ActorInfo) const
{
	if (!ActorInfo)
	{
		return nullptr;
	}
	
	if (APRPlayerCharacter* PlayerCharacter = Cast<APRPlayerCharacter>(ActorInfo->AvatarActor.Get()))
	{
		return PlayerCharacter->GetWeaponManager();
	}
	
	return nullptr;
}

UPRWeaponDataAsset* UPRGameplayAbility::GetActiveWeaponData(const FGameplayAbilityActorInfo* ActorInfo) const
{
	if (auto WeaponManager = GetWeaponManager(ActorInfo))
	{
		const FPRWeaponVisualInfo& CurrentWeaponVisualInfo = WeaponManager->GetCurrentWeaponVisualInfo();
		return CurrentWeaponVisualInfo.WeaponData;
	}
	
	return nullptr;
}

FGameplayEffectSpecHandle UPRGameplayAbility::MakePlayerEffectSpec(const FHitResult* HitResult, float Damage,
	float GroggyDamage)
{
	switch (PlayerAbilityType)
	{
	case EPRPlayerAbilityType::Mod:
		return MakeModEffectSpec(Damage, GroggyDamage, HitResult);
	case EPRPlayerAbilityType::Weapon:
		return MakeWeaponEffectSpec(HitResult);
	default:
		return FGameplayEffectSpecHandle();
	}
}

FGameplayEffectSpecHandle UPRGameplayAbility::MakeWeaponEffectSpec(const FHitResult* HitResult) const
{
	const UPRAbilitySystemRegistry* Registry = UPRAssetManager::Get().GetAbilitySystemRegistry();
	if (!IsValid(Registry) || !IsValid(Registry->DamageGE_FromWeapon))
	{
		return FGameplayEffectSpecHandle();
	}

	const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(
		GetCurrentAbilitySpecHandle(),
		GetCurrentActorInfo(),
		GetCurrentActivationInfo(),
		Registry->DamageGE_FromWeapon);

	if (!SpecHandle.IsValid())
	{
		return FGameplayEffectSpecHandle();
	}

	// HitResult가 있으면 EffectContext에 포함시켜 ExecCalc에서 부위 판정에 활용한다
	if (HitResult != nullptr && HitResult->bBlockingHit)
	{
		SpecHandle.Data->GetContext().AddHitResult(*HitResult, true);
	}

	return SpecHandle;
}

FGameplayEffectSpecHandle UPRGameplayAbility::MakeModEffectSpec(float Damage, float GroggyDamage,
	const FHitResult* HitResult) const
{
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(SourceASC))
	{
		return FGameplayEffectSpecHandle();
	}

	const UPRAbilitySystemRegistry* Registry = UPRAssetManager::Get().GetAbilitySystemRegistry();
	if (!IsValid(Registry) || !IsValid(Registry->DamageGE_FromMod))
	{
		return FGameplayEffectSpecHandle();
	}

	const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(
		GetCurrentAbilitySpecHandle(),
		GetCurrentActorInfo(),
		GetCurrentActivationInfo(),
		Registry->DamageGE_FromMod);

	if (!SpecHandle.IsValid())
	{
		return FGameplayEffectSpecHandle();
	}

	// 모드 스킬의 데미지와 그로기 데미지를 SetByCaller로 전달한다
	if (Damage > 0.0f)
	{
		SpecHandle.Data->SetSetByCallerMagnitude(PRCombatGameplayTags::SetByCaller_Damage, Damage);
	}

	if (GroggyDamage > 0.0f)
	{
		SpecHandle.Data->SetSetByCallerMagnitude(PRCombatGameplayTags::SetByCaller_GroggyDamage, GroggyDamage);
	}

	// HitResult가 있으면 EffectContext에 포함시켜 ExecCalc에서 부위 판정에 활용한다
	if (HitResult != nullptr && HitResult->bBlockingHit)
	{
		SpecHandle.Data->GetContext().AddHitResult(*HitResult, true);
	}

	return SpecHandle;
}

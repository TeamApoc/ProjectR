// Copyright ProjectR. All Rights Reserved.

#include "PRGA_Mod.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "DrawDebugHelpers.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Weapon.h"
#include "ProjectR/AbilitySystem/Data/PRAbilitySystemRegistry.h"
#include "ProjectR/AbilitySystem/Effects/PRGE_ModCost_GaugeDuration.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/Combat/PRCombatGameplayTags.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/System/PRAssetManager.h"
#include "ProjectR/Utils/PRGameplayStatics.h"
#include "ProjectR/Weapon/Actors/PRWeaponActor.h"
#include "ProjectR/Weapon/Components/PRWeaponManagerComponent.h"
#include "ProjectR/Weapon/Data/PRWeaponModDataAsset.h"
#include "ProjectR/Weapon/Items/PRItemInstance_Weapon.h"

UPRGA_Mod::UPRGA_Mod()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	// ========= Mod Base  =============
	InputTag = PRGameplayTags::Input_Ability_Mod;

}

/*~ UGameplayAbility Interface ~*/

void UPRGA_Mod::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// 활성 무기 캐싱. PRGA_Fire와 동일 패턴
	if (APRPlayerCharacter* PlayerCharacter = GetPRCharacter<APRPlayerCharacter>())
	{
		if (UPRWeaponManagerComponent* WeaponManager = PlayerCharacter->GetWeaponManager())
		{
			CachedWeaponManager = WeaponManager;
			CurrentWeapon = WeaponManager->GetActiveWeaponActor();
		}
	}
	
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

// ========= Mod Base  =============

bool UPRGA_Mod::CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags))
	{
		return false;
	}

	bool bHasCost = true;
	bool bIsBlocked = false;
	if (ModCostPolicy == EPRModCostPolicyType::Stack)
	{
		bHasCost = HasModStackCost(ActorInfo);
	}
	else if (ModCostPolicy == EPRModCostPolicyType::GaugeDuration)
	{
		bIsBlocked = HasActiveModGaugeLock(ActorInfo);
		bHasCost = !bIsBlocked && ModDuration > 0.0f && HasFullModGaugeCost(ActorInfo);
	}

	if (bIsBlocked && OptionalRelevantTags)
	{
		OptionalRelevantTags->AddTag(PRGameplayTags::Fail_Blocked);
	}
	else if (!bHasCost && OptionalRelevantTags)
	{
		OptionalRelevantTags->AddTag(PRGameplayTags::Fail_Cost);
	}

	return bHasCost;
}

void UPRGA_Mod::ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	Super::ApplyCost(Handle, ActorInfo, ActivationInfo);

	if (ModCostPolicy == EPRModCostPolicyType::Stack)
	{
		ApplyModStackCost(ActorInfo);
	}
	else if (ModCostPolicy == EPRModCostPolicyType::GaugeDuration)
	{
		ApplyModGaugeDurationCost(ActorInfo);
	}
}

void UPRGA_Mod::SetModCostPolicy(EPRModCostPolicyType InModCostType)
{
	ModCostPolicy = InModCostType;
}

bool UPRGA_Mod::TryGetCurrentModCostContext(const FGameplayAbilityActorInfo* ActorInfo, EPRWeaponSlotType& OutSlotType,
	UAbilitySystemComponent*& OutASC, UPRItemInstance_Weapon*& OutWeaponInstance) const
{
	OutSlotType = EPRWeaponSlotType::None;
	OutASC = nullptr;
	OutWeaponInstance = nullptr;

	if (!ActorInfo)
	{
		return false;
	}

	OutASC = ActorInfo->AbilitySystemComponent.Get();
	if (!IsValid(OutASC))
	{
		return false;
	}

	const APRPlayerCharacter* PlayerCharacter = Cast<APRPlayerCharacter>(ActorInfo->AvatarActor.Get());
	if (!IsValid(PlayerCharacter))
	{
		return false;
	}

	UPRWeaponManagerComponent* WeaponManager = PlayerCharacter->GetWeaponManager();
	if (!IsValid(WeaponManager))
	{
		return false;
	}

	OutSlotType = WeaponManager->GetCurrentWeaponSlot();
	if (OutSlotType == EPRWeaponSlotType::None)
	{
		return false;
	}

	OutWeaponInstance = WeaponManager->GetWeaponInstanceBySlotType(OutSlotType);
	return IsValid(OutWeaponInstance);
}

bool UPRGA_Mod::HasModStackCost(const FGameplayAbilityActorInfo* ActorInfo) const
{
	EPRWeaponSlotType SlotType = EPRWeaponSlotType::None;
	UAbilitySystemComponent* ASC = nullptr;
	UPRItemInstance_Weapon* WeaponInstance = nullptr;
	if (!TryGetCurrentModCostContext(ActorInfo, SlotType, ASC, WeaponInstance))
	{
		return false;
	}

	const FGameplayAttribute StackAttribute = GetModStackAttribute(SlotType);
	if (!StackAttribute.IsValid())
	{
		return false;
	}

	return ASC->GetNumericAttribute(StackAttribute) >= 1.0f;
}

bool UPRGA_Mod::HasFullModGaugeCost(const FGameplayAbilityActorInfo* ActorInfo) const
{
	EPRWeaponSlotType SlotType = EPRWeaponSlotType::None;
	UAbilitySystemComponent* ASC = nullptr;
	UPRItemInstance_Weapon* WeaponInstance = nullptr;
	if (!TryGetCurrentModCostContext(ActorInfo, SlotType, ASC, WeaponInstance))
	{
		return false;
	}

	const FGameplayAttribute MaxGaugeAttribute = GetMaxModGaugeAttribute(SlotType);
	if (!MaxGaugeAttribute.IsValid())
	{
		return false;
	}

	const FGameplayAttribute GaugeAttribute = GetModGaugeAttribute(SlotType);
	if (!GaugeAttribute.IsValid())
	{
		return false;
	}

	const float MaxGauge = ASC->GetNumericAttribute(MaxGaugeAttribute);
	return MaxGauge > 0.0f && ASC->GetNumericAttribute(GaugeAttribute) >= MaxGauge - KINDA_SMALL_NUMBER;
}

bool UPRGA_Mod::HasActiveModGaugeLock(const FGameplayAbilityActorInfo* ActorInfo) const
{
	EPRWeaponSlotType SlotType = EPRWeaponSlotType::None;
	UAbilitySystemComponent* ASC = nullptr;
	UPRItemInstance_Weapon* WeaponInstance = nullptr;
	if (!TryGetCurrentModCostContext(ActorInfo, SlotType, ASC, WeaponInstance))
	{
		return false;
	}

	if (SlotType == EPRWeaponSlotType::Primary)
	{
		return ASC->HasMatchingGameplayTag(PRGameplayTags::State_Mod_Primary_GaugeLocked);
	}

	if (SlotType == EPRWeaponSlotType::Secondary)
	{
		return ASC->HasMatchingGameplayTag(PRGameplayTags::State_Mod_Secondary_GaugeLocked);
	}

	return false;
}

void UPRGA_Mod::ApplyModStackCost(const FGameplayAbilityActorInfo* ActorInfo) const
{
	EPRWeaponSlotType SlotType = EPRWeaponSlotType::None;
	UAbilitySystemComponent* ASC = nullptr;
	UPRItemInstance_Weapon* WeaponInstance = nullptr;
	if (!TryGetCurrentModCostContext(ActorInfo, SlotType, ASC, WeaponInstance))
	{
		return;
	}

	const FGameplayAttribute StackAttribute = GetModStackAttribute(SlotType);
	if (!StackAttribute.IsValid())
	{
		return;
	}

	const float CurrentStack = ASC->GetNumericAttribute(StackAttribute);
	ASC->SetNumericAttributeBase(StackAttribute, FMath::Max(CurrentStack - 1.0f, 0.0f));
}

void UPRGA_Mod::ApplyModGaugeDurationCost(const FGameplayAbilityActorInfo* ActorInfo) const
{
	EPRWeaponSlotType SlotType = EPRWeaponSlotType::None;
	UAbilitySystemComponent* ASC = nullptr;
	UPRItemInstance_Weapon* WeaponInstance = nullptr;
	if (!TryGetCurrentModCostContext(ActorInfo, SlotType, ASC, WeaponInstance))
	{
		return;
	}

	const FGameplayAttribute MaxGaugeAttribute = GetMaxModGaugeAttribute(SlotType);
	if (!MaxGaugeAttribute.IsValid() || ASC->GetNumericAttribute(MaxGaugeAttribute) <= 0.0f || ModDuration <= 0.0f)
	{
		return;
	}

	const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(
		GetCurrentAbilitySpecHandle(),
		GetCurrentActorInfo(),
		GetCurrentActivationInfo(),
		UPRGE_ModCost_GaugeDuration::StaticClass());
	if (!SpecHandle.IsValid())
	{
		return;
	}

	SpecHandle.Data->SetSetByCallerMagnitude(PRCombatGameplayTags::SetByCaller_ModDuration, ModDuration);

	if (SlotType == EPRWeaponSlotType::Primary)
	{
		SpecHandle.Data->DynamicGrantedTags.AddTag(PRGameplayTags::State_Mod_Primary_GaugeLocked);
	}
	else if (SlotType == EPRWeaponSlotType::Secondary)
	{
		SpecHandle.Data->DynamicGrantedTags.AddTag(PRGameplayTags::State_Mod_Secondary_GaugeLocked);
	}
	else
	{
		return;
	}

	ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

	if (UWorld* World = GetWorld())
	{
		WeaponInstance->ModEffectEndServerWorldTimeSeconds = World->GetTimeSeconds() + ModDuration;
	}
}

FGameplayAttribute UPRGA_Mod::GetModGaugeAttribute(EPRWeaponSlotType SlotType) const
{
	if (SlotType == EPRWeaponSlotType::Primary)
	{
		return UPRAttributeSet_Weapon::GetPrimaryModGaugeAttribute();
	}

	if (SlotType == EPRWeaponSlotType::Secondary)
	{
		return UPRAttributeSet_Weapon::GetSecondaryModGaugeAttribute();
	}

	return FGameplayAttribute();
}

FGameplayAttribute UPRGA_Mod::GetMaxModGaugeAttribute(EPRWeaponSlotType SlotType) const
{
	if (SlotType == EPRWeaponSlotType::Primary)
	{
		return UPRAttributeSet_Weapon::GetPrimaryMaxModGaugeAttribute();
	}

	if (SlotType == EPRWeaponSlotType::Secondary)
	{
		return UPRAttributeSet_Weapon::GetSecondaryMaxModGaugeAttribute();
	}

	return FGameplayAttribute();
}

FGameplayAttribute UPRGA_Mod::GetModStackAttribute(EPRWeaponSlotType SlotType) const
{
	if (SlotType == EPRWeaponSlotType::Primary)
	{
		return UPRAttributeSet_Weapon::GetPrimaryModStackAttribute();
	}

	if (SlotType == EPRWeaponSlotType::Secondary)
	{
		return UPRAttributeSet_Weapon::GetSecondaryModStackAttribute();
	}

	return FGameplayAttribute();
}

FGameplayAttribute UPRGA_Mod::GetMaxModStackAttribute(EPRWeaponSlotType SlotType) const
{
	if (SlotType == EPRWeaponSlotType::Primary)
	{
		return UPRAttributeSet_Weapon::GetPrimaryMaxModStackAttribute();
	}

	if (SlotType == EPRWeaponSlotType::Secondary)
	{
		return UPRAttributeSet_Weapon::GetSecondaryMaxModStackAttribute();
	}

	return FGameplayAttribute();
}

/*~ 조준/총구 헬퍼 ~*/

FVector UPRGA_Mod::GetMuzzleLocation() const
{
	if (!CurrentWeapon.IsValid())
	{
		if (APRPlayerCharacter* PlayerCharacter = GetPRCharacter<APRPlayerCharacter>())
		{
			if (UPRWeaponManagerComponent* WeaponManager = PlayerCharacter->GetWeaponManager())
			{
				CurrentWeapon = WeaponManager->GetActiveWeaponActor();
			}
		}
	}
	
	if (CurrentWeapon.IsValid())
	{
		return CurrentWeapon->GetMuzzleTransform().GetLocation();
	}
	
	// Fallback: 무기 캐시가 없으면 아바타 정면 50cm 지점
	if (AActor* AvatarActor = GetAvatarActorFromActorInfo())
	{
		return AvatarActor->GetActorLocation() + AvatarActor->GetActorForwardVector() * 50.f;
	}

	return FVector::ZeroVector;
}

FTransform UPRGA_Mod::GetMuzzleTransform() const
{
	if (!CurrentWeapon.IsValid())
	{
		if (APRPlayerCharacter* PlayerCharacter = GetPRCharacter<APRPlayerCharacter>())
		{
			if (UPRWeaponManagerComponent* WeaponManager = PlayerCharacter->GetWeaponManager())
			{
				CurrentWeapon = WeaponManager->GetActiveWeaponActor();
			}
		}
	}
	
	if (CurrentWeapon.IsValid())
	{
		return CurrentWeapon->GetMuzzleTransform();
	}
	
	// Fallback: 무기 캐시가 없으면 아바타 정면 50cm 지점
	if (AActor* AvatarActor = GetAvatarActorFromActorInfo())
	{
		FTransform ActorTransform = AvatarActor->GetActorTransform();
		ActorTransform.SetLocation(AvatarActor->GetActorLocation() + AvatarActor->GetActorForwardVector() * 50.f);
		return ActorTransform;
	}

	return FTransform::Identity;
}

FPRFireViewpoint UPRGA_Mod::GetFireViewpoint() const
{
	FPRFireViewpoint View;

	if (!ensureMsgf(GetActorInfo().IsLocallyControlled(), TEXT("Viewpoint는 로컬에서만 유효.")))
	{
		return View;
	}

	UPRGameplayStatics::GetPawnViewpoint(Cast<APawn>(GetAvatarActorFromActorInfo()), View.Location, View.Rotation);
	return View;
}

FVector UPRGA_Mod::ResolveAimPoint(const FPRFireViewpoint& View, float InMaxTraceDistance) const
{
	APawn* OwnerPawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	TArray<AActor*> IgnoredActors;
	if (AActor* AvatarActor = GetAvatarActorFromActorInfo())
	{
		IgnoredActors.Add(AvatarActor);
	}
	const FVector AimPoint = UPRGameplayStatics::ResolveCameraAimPoint(OwnerPawn, InMaxTraceDistance, AimTraceChannel.GetValue(), IgnoredActors);

	// 디버그: 카메라 트레이스 시안색
	if (bDrawCameraTrace)
	{
		if (UWorld* World = GetWorld())
		{
			DrawDebugLine(World, View.Location, AimPoint, FColor::Cyan, false, DebugDrawDuration, 0, 0.5f);
		}
	}

	return AimPoint;
}

/*~ 데미지 적용 ~*/

void UPRGA_Mod::ApplyDamage(AActor* TargetActor, float Damage, float GroggyDamage, const FHitResult* HitResult)
{
	if (!IsValid(TargetActor))
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(SourceASC))
	{
		return;
	}

	const FGameplayEffectSpecHandle SpecHandle = MakeModEffectSpec(Damage, GroggyDamage, HitResult);
	if (!SpecHandle.IsValid())
	{
		return;
	}

	// 대상 ASC에 GE 적용
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	if (IsValid(TargetASC))
	{
		SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	}
}

FGameplayEffectSpecHandle UPRGA_Mod::MakeModEffectSpec(float Damage, float GroggyDamage, const FHitResult* HitResult) const
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

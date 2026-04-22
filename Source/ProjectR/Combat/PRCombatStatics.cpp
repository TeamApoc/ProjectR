// Copyright ProjectR. All Rights Reserved.

#include "PRCombatStatics.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameplayEffect.h"
#include "ProjectR/AbilitySystem/Effects/PRGameplayEffect_Damage.h"
#include "ProjectR/Combat/PRCombatGameplayTags.h"
#include "ProjectR/PRGameplayTags.h"

UAbilitySystemComponent* UPRCombatStatics::FindAbilitySystemComponent(const AActor* Actor)
{
	return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(const_cast<AActor*>(Actor));
}

FPRDamageRegionInfo UPRCombatStatics::ResolveDamageRegion(const FHitResult& HitResult, const AActor* TargetActor)
{
	return ResolveDamageRegionInternal(HitResult, FindAbilitySystemComponent(TargetActor));
}

bool UPRCombatStatics::IsCoreWeakpointOpen(const AActor* TargetActor)
{
	const UAbilitySystemComponent* TargetASC = FindAbilitySystemComponent(TargetActor);
	return IsValid(TargetASC) && TargetASC->HasMatchingGameplayTag(PRGameplayTags::State_Boss_WeakpointOpen_Core);
}

bool UPRCombatStatics::ApplyDamageEffect(const FPRDamageContext& DamageContext,
	TSubclassOf<UGameplayEffect> DamageEffectClass)
{
	// 피해 적용은 서버 권한 타겟에서만 수행한다.
	// 클라이언트에서 같은 계산을 반복하면 체력/그로기 수치가 엇갈릴 수 있다.
	if (!IsValid(DamageContext.SourceActor)
		|| !IsValid(DamageContext.TargetActor)
		|| !DamageContext.HasValidPayload()
		|| !DamageContext.TargetActor->HasAuthority())
	{
		return false;
	}

	UAbilitySystemComponent* SourceASC = FindAbilitySystemComponent(DamageContext.SourceActor);
	UAbilitySystemComponent* TargetASC = FindAbilitySystemComponent(DamageContext.TargetActor);
	if (!IsValid(SourceASC) || !IsValid(TargetASC))
	{
		return false;
	}

	TSubclassOf<UGameplayEffect> ResolvedDamageEffectClass = DamageEffectClass;
	if (!IsValid(ResolvedDamageEffectClass))
	{
		ResolvedDamageEffectClass = UPRGameplayEffect_Damage::StaticClass();
	}

	FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
	EffectContext.AddInstigator(DamageContext.SourceActor,
		IsValid(DamageContext.EffectCauser) ? DamageContext.EffectCauser : DamageContext.SourceActor);

	// SourceObject와 HitResult를 Context에 넣어 ExecutionCalculation에서 부위 판정까지 이어가게 한다.
	if (IsValid(DamageContext.SourceObject))
	{
		EffectContext.AddSourceObject(DamageContext.SourceObject);
	}

	if (DamageContext.HitResult.bBlockingHit)
	{
		EffectContext.AddHitResult(DamageContext.HitResult, true);
	}

	const float AbilityLevel = FMath::Max(DamageContext.AbilityLevel, 1.0f);
	const FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(ResolvedDamageEffectClass, AbilityLevel, EffectContext);
	if (!SpecHandle.IsValid())
	{
		return false;
	}

	if (DamageContext.Damage > 0.0f)
	{
		// 실제 수치는 SetByCaller로 전달한다. GE 클래스는 공용으로 재사용할 수 있다.
		SpecHandle.Data->SetSetByCallerMagnitude(PRCombatGameplayTags::SetByCaller_Damage, DamageContext.Damage);
	}

	if (DamageContext.GroggyDamage > 0.0f)
	{
		SpecHandle.Data->SetSetByCallerMagnitude(PRCombatGameplayTags::SetByCaller_GroggyDamage, DamageContext.GroggyDamage);
	}

	if (DamageContext.SourceAbilityTag.IsValid())
	{
		SpecHandle.Data->AddDynamicAssetTag(DamageContext.SourceAbilityTag);
	}

	SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	return true;
}

bool UPRCombatStatics::FindTaggedRegion(const UPrimitiveComponent* HitComponent, const TCHAR* Prefix, FName& OutRegionTag)
{
	if (!IsValid(HitComponent))
	{
		return false;
	}

	for (const FName& ComponentTag : HitComponent->ComponentTags)
	{
		if (ComponentTag.ToString().StartsWith(Prefix))
		{
			OutRegionTag = ComponentTag;
			return true;
		}
	}

	return false;
}

FPRDamageRegionInfo UPRCombatStatics::ResolveDamageRegionInternal(const FHitResult& HitResult,
	const UAbilitySystemComponent* TargetASC)
{
	FPRDamageRegionInfo RegionInfo;

	const UPrimitiveComponent* HitComponent = HitResult.GetComponent();
	if (!IsValid(HitComponent))
	{
		return RegionInfo;
	}

	FName WeakpointTag = NAME_None;
	if (FindTaggedRegion(HitComponent, TEXT("Weakpoint."), WeakpointTag))
	{
		// 보스 코어 약점은 별도 태그로 열린 상태일 때만 약점으로 인정한다.
		const bool bIsCoreWeakpoint = WeakpointTag == TEXT("Weakpoint.Core");
		if (!bIsCoreWeakpoint
			|| (IsValid(TargetASC) && TargetASC->HasMatchingGameplayTag(PRGameplayTags::State_Boss_WeakpointOpen_Core)))
		{
			RegionInfo.RegionType = EPRDamageRegionType::Weakpoint;
			RegionInfo.RegionTag = WeakpointTag;
			return RegionInfo;
		}
	}

	FName ArmorTag = NAME_None;
	if (FindTaggedRegion(HitComponent, TEXT("Armor."), ArmorTag))
	{
		RegionInfo.RegionType = EPRDamageRegionType::Armor;
		RegionInfo.RegionTag = ArmorTag;
	}

	return RegionInfo;
}

// Copyright ProjectR. All Rights Reserved.

#include "PRGA_Mod.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "DrawDebugHelpers.h"
#include "ProjectR/AbilitySystem/Data/PRAbilitySystemRegistry.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/Combat/PRCombatGameplayTags.h"
#include "ProjectR/System/PRAssetManager.h"
#include "ProjectR/Weapon/Actors/PRWeaponActor.h"
#include "ProjectR/Weapon/Components/PRWeaponManagerComponent.h"

UPRGA_Mod::UPRGA_Mod()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

/*~ UGameplayAbility Interface ~*/

void UPRGA_Mod::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 활성 무기 캐싱. PRGA_Fire와 동일 패턴
	if (APRPlayerCharacter* PlayerCharacter = GetPRCharacter<APRPlayerCharacter>())
	{
		if (UPRWeaponManagerComponent* WeaponManager = PlayerCharacter->GetWeaponManager())
		{
			CachedWeaponManager = WeaponManager;
			CurrentWeapon = WeaponManager->GetActiveWeaponActor();
		}
	}
}

/*~ 조준/총구 헬퍼 ~*/

FVector UPRGA_Mod::GetMuzzleLocation() const
{
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

	// TPS 숄더뷰: SpringArm/카메라 오프셋이 반영된 실제 카메라 위치/회전 사용
	APlayerController* PC = Cast<APlayerController>(GetActorInfo().PlayerController.Get());
	if (IsValid(PC) && IsValid(PC->PlayerCameraManager))
	{
		View.Location = PC->PlayerCameraManager->GetCameraLocation();
		View.Rotation = PC->PlayerCameraManager->GetCameraRotation();
		return View;
	}

	// Fallback: 컨트롤러 없을 때 액터 눈높이 기준
	if (APawn* OwnerPawn = Cast<APawn>(GetAvatarActorFromActorInfo()))
	{
		OwnerPawn->GetActorEyesViewPoint(View.Location, View.Rotation);
	}

	return View;
}

FVector UPRGA_Mod::ResolveAimPoint(const FPRFireViewpoint& View, float InMaxTraceDistance) const
{
	const FVector CamStart = View.Location;
	const FVector CamEnd = CamStart + View.Rotation.Vector() * InMaxTraceDistance;

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return CamEnd;
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(PRModAimTrace), false);
	if (AActor* AvatarActor = GetAvatarActorFromActorInfo())
	{
		Params.AddIgnoredActor(AvatarActor);
	}

	FHitResult AimHit;
	World->LineTraceSingleByChannel(AimHit, CamStart, CamEnd, AimTraceChannel.GetValue(), Params);

	// 디버그: 카메라 트레이스 시안색
	if (bDrawCameraTrace)
	{
		const FVector AimDrawEnd = AimHit.bBlockingHit ? AimHit.ImpactPoint : CamEnd;
		DrawDebugLine(World, CamStart, AimDrawEnd, FColor::Cyan, false, DebugDrawDuration, 0, 0.5f);
	}

	return AimHit.bBlockingHit ? AimHit.ImpactPoint : CamEnd;
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

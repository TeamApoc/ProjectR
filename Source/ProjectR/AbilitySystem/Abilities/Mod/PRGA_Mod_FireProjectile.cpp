// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#include "PRGA_Mod_FireProjectile.h"

#include "AbilitySystemComponent.h"
#include "NiagaraSystem.h"
#include "ProjectR/AbilitySystem/Tasks/PRAT_SpawnPredictedProjectile.h"
#include "ProjectR/Combat/PRCombatGameplayTags.h"
#include "ProjectR/Projectile/PRProjectileBase.h"
#include "ProjectR/Utils/PRGameplayStatics.h"
#include "ProjectR/Weapon/Components/PRWeaponManagerComponent.h"

UPRGA_Mod_FireProjectile::UPRGA_Mod_FireProjectile()
{
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
}

/*~ UGameplayAbility Interface ~*/

void UPRGA_Mod_FireProjectile::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/true);
		return;
	}
	
	// 총구 이펙트 스폰. unreliable multicast
	if (IsValid(MuzzleVFX) && CachedWeaponManager.IsValid())
	{
		CachedWeaponManager->PlayWeaponNiagaraEffect(EPRWeaponEffectType::ProjectileLaunch,MuzzleVFX);
		
		if (HasAuthority(&ActivationInfo))
		{
			CachedWeaponManager->Multicast_PlayWeaponNiagaraEffect(EPRWeaponEffectType::ProjectileLaunch,MuzzleVFX);
		}
	}

	FTransform LaunchTransform = GetProjectileLaunchTransform();
	FireProjectile(LaunchTransform.GetLocation(), LaunchTransform.Rotator());
}

/*~ 투사체 발사 ~*/

void UPRGA_Mod_FireProjectile::FireProjectile(FVector SpawnLocation, FRotator SpawnRotation)
{
	if (!IsValid(ProjectileClass))
	{
		return;
	}

	UPRAT_SpawnPredictedProjectile* Task = UPRAT_SpawnPredictedProjectile::SpawnPredictedProjectile(
		this, ProjectileClass, SpawnLocation, SpawnRotation);
	if (!IsValid(Task))
	{
		return;
	}

	// AbilityTask 결과 콜백 바인딩. 결과 통지는 OnProjectileSpawnSuccess/Failed 핸들러로 일원화
	Task->OnSuccess.BindDynamic(this, &UPRGA_Mod_FireProjectile::OnProjectileSpawnSuccess);
	Task->OnFailed.BindDynamic(this, &UPRGA_Mod_FireProjectile::OnProjectileSpawnFailed);

	// ReadyForActivation 호출 시 AT::Activate 실행, 클라/서버 각자의 스폰 흐름 진행
	Task->ReadyForActivation();
}

FTransform UPRGA_Mod_FireProjectile::GetProjectileLaunchTransform() const
{
	if (!GetActorInfo().IsLocallyControlled())
	{
		return FTransform();
	}

	APawn* OwnerPawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	TArray<AActor*> IgnoredActors;
	if (AActor* AvatarActor = GetAvatarActorFromActorInfo())
	{
		IgnoredActors.Add(AvatarActor);
	}
	return UPRGameplayStatics::ResolveProjectileLaunchTransform(OwnerPawn, GetMuzzleLocation(), MaxTraceDistance, FireTraceChannel.GetValue(), IgnoredActors);
}

/*~ 스폰 결과 콜백 ~*/

void UPRGA_Mod_FireProjectile::OnProjectileSpawnSuccess(APRProjectileBase* SpawnedProjectile)
{
	if (!IsValid(SpawnedProjectile))
	{
		K2_EndAbility();
		return;
	}

	// 서버 권위에 한해 모드 데미지 GE Spec 부여. Predicted 클라이언트는 데미지 적용 책임 없음
	if (HasAuthority(&CurrentActivationInfo))
	{
		const FGameplayEffectSpecHandle SpecHandle = MakeModEffectSpec(Damage, GroggyDamage);
		if (SpecHandle.IsValid())
		{
			SpawnedProjectile->InitGameplayEffectSpec(SpecHandle);
		}
	}

	K2_OnProjectileSpawnSuccess(SpawnedProjectile);
	K2_EndAbility();
}

void UPRGA_Mod_FireProjectile::OnProjectileSpawnFailed(APRProjectileBase* SpawnedProjectile)
{
	K2_OnProjectileSpawnFailed(SpawnedProjectile);
	K2_EndAbility();
}
/*~ EffectSpec 오버라이드 ~*/

FGameplayEffectSpecHandle UPRGA_Mod_FireProjectile::MakeModEffectSpec(float InDamage, float InGroggyDamage, const FHitResult* HitResult) const
{
	// Override가 비어있으면 베이스 흐름(Registry의 DamageGE_FromMod) 사용
	if (!IsValid(ProjectileEffectOverride))
	{
		return Super::MakeModEffectSpec(InDamage, InGroggyDamage, HitResult);
	}

	const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(
		GetCurrentAbilitySpecHandle(),
		GetCurrentActorInfo(),
		GetCurrentActivationInfo(),
		ProjectileEffectOverride);

	if (!SpecHandle.IsValid())
	{
		return FGameplayEffectSpecHandle();
	}

	if (InDamage > 0.0f)
	{
		SpecHandle.Data->SetSetByCallerMagnitude(PRCombatGameplayTags::SetByCaller_Damage, InDamage);
	}

	if (InGroggyDamage > 0.0f)
	{
		SpecHandle.Data->SetSetByCallerMagnitude(PRCombatGameplayTags::SetByCaller_GroggyDamage, InGroggyDamage);
	}

	if (HitResult != nullptr && HitResult->bBlockingHit)
	{
		SpecHandle.Data->GetContext().AddHitResult(*HitResult, true);
	}

	return SpecHandle;
}

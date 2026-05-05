// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#include "PRGA_Mod_FireProjectile.h"

#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "ProjectR/AbilitySystem/Tasks/PRAT_SpawnPredictedProjectile.h"
#include "ProjectR/Projectile/PRProjectileBase.h"
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

	// 발사 트랜스폼 계산은 로컬 컨트롤러에서만 수행. 원격 서버는 AbilityTask가 클라 TargetData 수신
	FVector SpawnLocation = FVector::ZeroVector;
	FRotator SpawnRotation = FRotator::ZeroRotator;

	if (GetActorInfo().IsLocallyControlled())
	{
		const FPRFireViewpoint View = GetFireViewpoint();
		const FTransform MuzzleTransform = GetMuzzleTransform();
		SpawnLocation = MuzzleTransform.GetLocation();

		// 1차 카메라 트레이스로 조준 끝점 산출, 조준점 방향을 발사 회전으로 환산
		const FVector AimPoint = ResolveAimPoint(View, MaxTraceDistance);
		FVector LaunchDir = AimPoint - SpawnLocation;
		if (!LaunchDir.Normalize(KINDA_SMALL_NUMBER))
		{
			// 조준점이 총구 바로 앞/뒤 등 거리 0인 경우 카메라 정면 방향으로 폴백
			LaunchDir = View.Rotation.Vector();
		}

		SpawnRotation = LaunchDir.Rotation();
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

	FireProjectile(SpawnLocation, SpawnRotation);
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

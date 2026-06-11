// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (특성 기반 투사체 피해 보정 연동 구현)
// Author: 배유찬 (예측 투사체 발사 경로 프리뷰 및 발사 제어 구현)
#include "PRGA_FireProjectile.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/Projectile/PRProjectileBase.h"
#include "ProjectR/Projectile/PRFirePreviewComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogPRPathPreviewAbility, Log, All);

void UPRGA_FireProjectile::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	if (ActorInfo == nullptr)
	{
		UE_LOG(LogPRPathPreviewAbility, Warning, TEXT("OnGiveAbility 중단. ActorInfo 없음, Ability=%s"),
			*GetNameSafe(this));
		return;
	}

	UE_LOG(LogPRPathPreviewAbility, Log,
		TEXT("OnGiveAbility 진입. Ability=%s, bShouldPreviewPath=%d, bLocallyControlledPlayer=%d, Avatar=%s"),
		*GetNameSafe(this),
		bShouldPreviewPath,
		ActorInfo->IsLocallyControlledPlayer(),
		*GetNameSafe(ActorInfo->AvatarActor.Get()));
	
	// 어빌리티 부여는 서버/소유 클라이언트 모두에서 호출되지만 예측 경로 표시는 로컬 시각 효과이므로 로컬 컨트롤된 폰에서만 의미가 있음
	if (bShouldPreviewPath && ActorInfo->IsLocallyControlledPlayer())
	{
		APRPlayerCharacter* PlayerChar = Cast<APRPlayerCharacter>(ActorInfo->AvatarActor.Get());
		if (!IsValid(PlayerChar))
		{
			UE_LOG(LogPRPathPreviewAbility, Warning, TEXT("OnGiveAbility 중단. PlayerChar 없음, Avatar=%s"),
				*GetNameSafe(ActorInfo->AvatarActor.Get()));
			return;
		}

		UPRFirePreviewComponent* Preview = PlayerChar->FindComponentByClass<UPRFirePreviewComponent>();
		if (!IsValid(Preview))
		{
			UE_LOG(LogPRPathPreviewAbility, Warning, TEXT("OnGiveAbility 중단. PreviewComponent 없음, Player=%s"),
				*GetNameSafe(PlayerChar));
			return;
		}
		
		Preview->BindObject(this);

		// 디자이너가 어빌리티에 작성한 템플릿을 기반으로 작업
		FPRProjectilePreviewParams Params = PreviewParams;

		// ProjectileClass CDO의 PMC/Sphere 컴포넌트 기본값을 추출하여 시각/실제 비행 정합성 확보
		if (IsValid(ProjectileClass))
		{
			const APRProjectileBase* ProjectileCDO = ProjectileClass->GetDefaultObject<APRProjectileBase>();
			if (IsValid(ProjectileCDO))
			{
				if (const UProjectileMovementComponent* PMC = ProjectileCDO->FindComponentByClass<UProjectileMovementComponent>())
				{
					Params.InitialSpeed = PMC->InitialSpeed;
					Params.GravityScale = PMC->ProjectileGravityScale;
				}
				if (const USphereComponent* Sphere = ProjectileCDO->FindComponentByClass<USphereComponent>())
				{
					Params.CollisionRadius = Sphere->GetUnscaledSphereRadius();
				}
			}
		}

		// 무기 액터 바인딩은 별도 경로(PlayerCharacter의 State.Armed 태그 토글)에서 수행하므로 본 함수에서는 다루지 않음
		Preview->SetFireParams(Params);
		Preview->SetTrajectoryPreviewEnabled(true);

		UE_LOG(LogPRPathPreviewAbility, Log,
			TEXT("Preview 활성화 요청 완료. Ability=%s, Player=%s, Preview=%s, ProjectileClass=%s, InitialSpeed=%.2f, GravityScale=%.2f, CollisionRadius=%.2f"),
			*GetNameSafe(this),
			*GetNameSafe(PlayerChar),
			*GetNameSafe(Preview),
			*GetNameSafe(ProjectileClass.Get()),
			Params.InitialSpeed,
			Params.GravityScale,
			Params.CollisionRadius);
	}
	else
	{
		UE_LOG(LogPRPathPreviewAbility, Log,
			TEXT("Preview 활성화 생략. Ability=%s, bShouldPreviewPath=%d, bLocallyControlledPlayer=%d"),
			*GetNameSafe(this),
			bShouldPreviewPath,
			ActorInfo->IsLocallyControlledPlayer());
	}
}

void UPRGA_FireProjectile::OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnRemoveAbility(ActorInfo, Spec);

	if (ActorInfo == nullptr)
	{
		UE_LOG(LogPRPathPreviewAbility, Warning, TEXT("OnRemoveAbility 중단. ActorInfo 없음, Ability=%s"),
			*GetNameSafe(this));
		return;
	}

	UE_LOG(LogPRPathPreviewAbility, Log,
		TEXT("OnRemoveAbility 진입. Ability=%s, bShouldPreviewPath=%d, bLocallyControlledPlayer=%d, Avatar=%s"),
		*GetNameSafe(this),
		bShouldPreviewPath,
		ActorInfo->IsLocallyControlledPlayer(),
		*GetNameSafe(ActorInfo->AvatarActor.Get()));
	
	// 어빌리티 부여는 서버/소유 클라이언트 모두에서 호출되지만 예측 경로 표시는 로컬 시각 효과이므로 로컬 컨트롤된 폰에서만 의미가 있음
	if (bShouldPreviewPath && ActorInfo->IsLocallyControlledPlayer())
	{
		APRPlayerCharacter* PlayerChar = Cast<APRPlayerCharacter>(ActorInfo->AvatarActor.Get());
		if (!IsValid(PlayerChar))
		{
			UE_LOG(LogPRPathPreviewAbility, Warning, TEXT("OnRemoveAbility 중단. PlayerChar 없음, Avatar=%s"),
				*GetNameSafe(ActorInfo->AvatarActor.Get()));
			return;
		}

		UPRFirePreviewComponent* Preview = PlayerChar->FindComponentByClass<UPRFirePreviewComponent>();
		if (!IsValid(Preview) || Preview->GetBoundObject() != this)
		{
			UE_LOG(LogPRPathPreviewAbility, Warning,
				TEXT("OnRemoveAbility 중단. PreviewComponent 불일치, Player=%s, Preview=%s, BoundObject=%s, Ability=%s"),
				*GetNameSafe(PlayerChar),
				*GetNameSafe(Preview),
				IsValid(Preview) ? *GetNameSafe(Preview->GetBoundObject()) : TEXT("None"),
				*GetNameSafe(this));
			return;
		}
		
		Preview->SetTrajectoryPreviewEnabled(false);

		UE_LOG(LogPRPathPreviewAbility, Log, TEXT("Preview 비활성화 요청 완료. Ability=%s, Player=%s, Preview=%s"),
			*GetNameSafe(this),
			*GetNameSafe(PlayerChar),
			*GetNameSafe(Preview));
	}
	else
	{
		UE_LOG(LogPRPathPreviewAbility, Log,
			TEXT("Preview 비활성화 생략. Ability=%s, bShouldPreviewPath=%d, bLocallyControlledPlayer=%d"),
			*GetNameSafe(this),
			bShouldPreviewPath,
			ActorInfo->IsLocallyControlledPlayer());
	}
}

/*~ EffectSpec 오버라이드 ~*/

FGameplayEffectSpecHandle UPRGA_FireProjectile::MakeWeaponEffectSpec(const FHitResult* HitResult) const
{
	// Override가 비어있으면 베이스 흐름(Registry의 DamageGE_FromWeapon) 사용
	if (!IsValid(ProjectileEffectOverride))
	{
		return Super::MakeWeaponEffectSpec(HitResult);
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

	AddCurrentWeaponDamageData(SpecHandle);

	if (HitResult != nullptr && HitResult->bBlockingHit)
	{
		SpecHandle.Data->GetContext().AddHitResult(*HitResult, true);
	}

	return SpecHandle;
}

void UPRGA_FireProjectile::OnProjectileSpawnSuccess(APRProjectileBase* SpawnedProjectile)
{
	if (!IsValid(SpawnedProjectile))
	{
		return;
	}

	// 서버 권위에 한해 무기 데미지 GE Spec 부여. Predicted 클라이언트는 데미지 적용 책임 없음
	if (HasAuthority(&CurrentActivationInfo))
	{
		const FGameplayEffectSpecHandle SpecHandle = MakeWeaponEffectSpec();
		if (SpecHandle.IsValid())
		{
			SpawnedProjectile->InitGameplayEffectSpec(SpecHandle);
		}
	}
	
	Super::OnProjectileSpawnSuccess(SpawnedProjectile);
}

// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRGA_FireProjectile.h"

#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/Projectile/PRProjectileBase.h"
#include "ProjectR/Projectile/PRProjectileTrajectoryPreviewComponent.h"

void UPRGA_FireProjectile::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	if (ActorInfo == nullptr)
	{
		return;
	}
	
	// 어빌리티 부여는 서버/소유 클라이언트 모두에서 호출되지만 예측 경로 표시는 로컬 시각 효과이므로 로컬 컨트롤된 폰에서만 의미가 있음
	if (bShouldPreviewPath && ActorInfo->IsLocallyControlledPlayer())
	{
		APRPlayerCharacter* PlayerChar = Cast<APRPlayerCharacter>(ActorInfo->AvatarActor.Get());
		if (!IsValid(PlayerChar))
		{
			return;
		}

		UPRProjectileTrajectoryPreviewComponent* Preview = PlayerChar->FindComponentByClass<UPRProjectileTrajectoryPreviewComponent>();
		if (!IsValid(Preview))
		{
			return;
		}

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
		Preview->SetPreviewEnabled(true);
	}
}

void UPRGA_FireProjectile::OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnRemoveAbility(ActorInfo, Spec);
	
	// 어빌리티 부여는 서버/소유 클라이언트 모두에서 호출되지만 예측 경로 표시는 로컬 시각 효과이므로 로컬 컨트롤된 폰에서만 의미가 있음
	if (bShouldPreviewPath && ActorInfo->IsLocallyControlledPlayer())
	{
		APRPlayerCharacter* PlayerChar = Cast<APRPlayerCharacter>(ActorInfo->AvatarActor.Get());
		if (!IsValid(PlayerChar))
		{
			return;
		}

		UPRProjectileTrajectoryPreviewComponent* Preview = PlayerChar->FindComponentByClass<UPRProjectileTrajectoryPreviewComponent>();
		if (!IsValid(Preview))
		{
			return;
		}
		
		Preview->SetPreviewEnabled(false);
	}
}

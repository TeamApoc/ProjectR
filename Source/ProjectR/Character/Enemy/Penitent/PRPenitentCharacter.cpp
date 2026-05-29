// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#include "PRPenitentCharacter.h"

#include "MotionWarpingComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/Projectile/PRGroundBoxProjectileBase.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/AI/Components/PRSoldierArmoredDebugDrawComponent.h"

APRPenitentCharacter::APRPenitentCharacter()
{
	// 스탯 Registry / DT_EnemyStats의 RowName과 일치
	CharacterID = TEXT("Penitent");
	
	DebugDrawComponent = CreateDefaultSubobject<UPRSoldierArmoredDebugDrawComponent>(TEXT("DebugDrawComponent"));
	MotionWarpingComponent = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarpingComponent"));
	
	BarrierBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("BarrierArm"));
	BarrierBoom->SetupAttachment(RootComponent);
	BarrierBoom->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));
	BarrierBoom->TargetArmLength = -200;
	BarrierBoom->bUsePawnControlRotation = true;
	BarrierBoom->bEnableCameraRotationLag = true;
	BarrierBoom->CameraRotationLagSpeed = 5.f;
	
	BarrierAttachPoint = CreateDefaultSubobject<USceneComponent>(TEXT("BarrierAttachPoint"));
	BarrierAttachPoint->SetupAttachment(BarrierBoom);
	
}

void APRPenitentCharacter::HandleDeadTagChanged(bool bEntered)
{
	Super::HandleDeadTagChanged(bEntered);

	// 사망 진입 검사
	if (!bEntered || !HasAuthority())
	{
		return;
	}

	UPRAbilitySystemComponent* PenitentAbilitySystemComponent = GetEnemyAbilitySystemComponent();
	if (!IsValid(PenitentAbilitySystemComponent)
		|| !PenitentAbilitySystemComponent->HasMatchingGameplayTag(PRGameplayTags::State_Enemy_Penitent_BarrierSummon))
	{
		return;
	}

	CleanupSummonedBarrier(true);
}

void APRPenitentCharacter::SetSpawnedBarrierActor(APRGroundBoxProjectileBase* InBarrierActor)
{
	if (IsValid(SpawnedBarrierActor))
	{
		SpawnedBarrierActor->OnGroundBoxDestroyed.RemoveDynamic(this, &ThisClass::HandleSpawnedBarrierDestroyed);
	}

	SpawnedBarrierActor = InBarrierActor;

	if (IsValid(SpawnedBarrierActor))
	{
		SpawnedBarrierActor->OnGroundBoxDestroyed.AddUniqueDynamic(this, &ThisClass::HandleSpawnedBarrierDestroyed);
	}
}

void APRPenitentCharacter::ClearSpawnedBarrierActor(APRGroundBoxProjectileBase* BarrierActorToClear)
{
	if (IsValid(BarrierActorToClear) && SpawnedBarrierActor != BarrierActorToClear)
	{
		return;
	}

	if (IsValid(SpawnedBarrierActor))
	{
		SpawnedBarrierActor->OnGroundBoxDestroyed.RemoveDynamic(this, &ThisClass::HandleSpawnedBarrierDestroyed);
	}

	SpawnedBarrierActor = nullptr;
}

void APRPenitentCharacter::CleanupSummonedBarrier(bool bRequestBarrierEnd)
{
	APRGroundBoxProjectileBase* BarrierActor = SpawnedBarrierActor.Get();

	// 사망 소멸 요청
	if (bRequestBarrierEnd && HasAuthority() && IsValid(BarrierActor))
	{
		BarrierActor->RequestGroundBoxEnd();
	}

	ClearSpawnedBarrierActor(BarrierActor);

	UPRAbilitySystemComponent* PenitentAbilitySystemComponent = GetEnemyAbilitySystemComponent();
	if (!IsValid(PenitentAbilitySystemComponent))
	{
		return;
	}

	// 배리어 보유 상태 태그 정리
	PenitentAbilitySystemComponent->RemoveLooseGameplayTag(PRGameplayTags::State_Enemy_Penitent_BarrierSummon);
	PenitentAbilitySystemComponent->RemoveReplicatedLooseGameplayTag(PRGameplayTags::State_Enemy_Penitent_BarrierSummon);
}

bool APRPenitentCharacter::HasActiveBarrier() const
{
	return IsValid(SpawnedBarrierActor);
}

void APRPenitentCharacter::HandleSpawnedBarrierDestroyed(APRGroundBoxProjectileBase* DestroyedBarrier)
{
	if (SpawnedBarrierActor != DestroyedBarrier)
	{
		return;
	}

	CleanupSummonedBarrier(false);
}

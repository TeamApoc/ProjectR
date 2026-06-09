// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PREnemyProjectileActor.h"

#include "ProjectR/Character/Enemy/PREnemyBaseCharacter.h"

APREnemyProjectileActor::APREnemyProjectileActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void APREnemyProjectileActor::HandleHit_Implementation(UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	FVector NormalImpulse,
	const FHitResult& Hit)
{
	Super::HandleHit_Implementation(HitComponent, OtherActor, OtherComp, NormalImpulse, Hit);

	ApplyEffectToTargetWithHit(OtherActor, Hit);
}

bool APREnemyProjectileActor::ShouldIgnoreProjectileHit(AActor* OtherActor, UPrimitiveComponent* OtherComp, const FHitResult& Hit) const
{
	if (Super::ShouldIgnoreProjectileHit(OtherActor, OtherComp, Hit))
	{
		return true;
	}

	return IsValid(Cast<APREnemyBaseCharacter>(OtherActor));
}

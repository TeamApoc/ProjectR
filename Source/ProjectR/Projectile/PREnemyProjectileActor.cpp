// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PREnemyProjectileActor.h"

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

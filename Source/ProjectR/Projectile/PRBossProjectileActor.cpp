// Copyright ProjectR. All Rights Reserved.

#include "PRBossProjectileActor.h"

void APRBossProjectileActor::HandleHit_Implementation(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	ApplyEffectToTargetWithHit(OtherActor, Hit);
	Super::HandleHit_Implementation(HitComponent, OtherActor, OtherComp, NormalImpulse, Hit);
}

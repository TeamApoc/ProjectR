// Copyright ProjectR. All Rights Reserved.

#include "PRBarrierAbilityDataAsset.h"

#include "ProjectR/Projectile/PRBarrierAnchorActor.h"

UPRBarrierAbilityDataAsset::UPRBarrierAbilityDataAsset()
{
	// 기본 앵커 클래스
	BarrierAnchorActorClass = APRBarrierAnchorActor::StaticClass();
}

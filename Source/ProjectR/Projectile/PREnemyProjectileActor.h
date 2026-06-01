// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PRProjectileBase.h"
#include "PREnemyProjectileActor.generated.h"

class UPrimitiveComponent;

UCLASS()
class PROJECTR_API APREnemyProjectileActor : public APRProjectileBase
{
	GENERATED_BODY()

public:
	// 일반 적 투사체 기본값을 초기화한다.
	APREnemyProjectileActor();

protected:
	/*~ APRProjectileBase Interface ~*/
	virtual void HandleHit_Implementation(UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit) override;
	
	
};

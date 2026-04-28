// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "PRProjectileMovementComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTR_API UPRProjectileMovementComponent : public UProjectileMovementComponent
{
	GENERATED_BODY()

public:
	UPRProjectileMovementComponent();
};

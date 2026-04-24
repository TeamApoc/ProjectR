// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PRGA_Fire.h"
#include "PRGA_FireFullAuto.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTR_API UPRGA_FireFullAuto : public UPRGA_Fire
{
	GENERATED_BODY()
	
public:
	UPRGA_FireFullAuto();
	
public:
	

private:
	uint32 NextShotId = 0; // Local ID
};

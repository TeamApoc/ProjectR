// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PREnemyInterface.generated.h"

class UBehaviorTree;
class UPRAbilitySystemComponent;
class UPRPatternDataAsset;
class UPRPerceptionConfig;
class UPREnemyThreatComponent;

UINTERFACE(MinimalAPI)
class UPREnemyInterface : public UInterface
{
	GENERATED_BODY()
};

class PROJECTR_API IPREnemyInterface
{
	GENERATED_BODY()

public:
	virtual UPRAbilitySystemComponent* GetEnemyAbilitySystemComponent() const = 0;
	virtual UPREnemyThreatComponent* GetEnemyThreatComponent() const = 0;
	virtual UPRPatternDataAsset* GetPatternDataAsset() const = 0;
	virtual UPRPerceptionConfig* GetPerceptionConfig() const = 0;
	virtual UBehaviorTree* GetBehaviorTreeAsset() const = 0;
	virtual FVector GetHomeLocation() const = 0;
};

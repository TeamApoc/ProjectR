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

// AIController와 BT Task가 구체 Pawn 클래스에 의존하지 않도록 만드는 적 공통 인터페이스다.
UINTERFACE(MinimalAPI)
class UPREnemyInterface : public UInterface
{
	GENERATED_BODY()
};

class PROJECTR_API IPREnemyInterface
{
	GENERATED_BODY()

public:
	// 서버에서 Ability를 실행하고 태그/속성을 조회할 ASC다.
	virtual UPRAbilitySystemComponent* GetEnemyAbilitySystemComponent() const = 0;

	// 현재 공격 대상과 위협 목록을 관리하는 컴포넌트다.
	virtual UPREnemyThreatComponent* GetEnemyThreatComponent() const = 0;

	// BT가 패턴을 고를 때 사용하는 데이터 에셋이다.
	virtual UPRPatternDataAsset* GetPatternDataAsset() const = 0;

	// AI Perception 설정값을 담은 데이터 에셋이다.
	virtual UPRPerceptionConfig* GetPerceptionConfig() const = 0;

	// Possess 시 실행할 BehaviorTree다.
	virtual UBehaviorTree* GetBehaviorTreeAsset() const = 0;

	// 복귀 패턴에서 사용할 스폰/기준 위치다.
	virtual FVector GetHomeLocation() const = 0;
};

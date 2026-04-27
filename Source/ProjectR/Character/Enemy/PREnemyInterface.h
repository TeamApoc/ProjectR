// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PREnemyInterface.generated.h"

class UBehaviorTree;
class UPRAbilitySystemComponent;
class UPREnemyCombatDataAsset;
class UPRPatternDataAsset;
class UPRPerceptionConfig;
class UPREnemyThreatComponent;

// AIController와 BT Task가 구체 Pawn 클래스에 직접 의존하지 않도록 만드는 공용 인터페이스
UINTERFACE(MinimalAPI)
class UPREnemyInterface : public UInterface
{
	GENERATED_BODY()
};

class PROJECTR_API IPREnemyInterface
{
	GENERATED_BODY()

public:
	// 서버에서 Ability를 실행하고 태그/속성을 조회할 ASC
	virtual UPRAbilitySystemComponent* GetEnemyAbilitySystemComponent() const = 0;

	// 현재 공격 대상과 위협 목록을 관리하는 컴포넌트
	virtual UPREnemyThreatComponent* GetEnemyThreatComponent() const = 0;

	// BT가 패턴을 고를 때 사용하는 데이터 자산
	virtual UPRPatternDataAsset* GetPatternDataAsset() const = 0;

	// 적 전용 전투 데이터 자산
	virtual UPREnemyCombatDataAsset* GetCombatDataAsset() const = 0;

	// AI Perception 설정값을 담은 데이터 자산
	virtual UPRPerceptionConfig* GetPerceptionConfig() const = 0;

	// Possess 후 실행할 BehaviorTree
	virtual UBehaviorTree* GetBehaviorTreeAsset() const = 0;

	// 복귀 행동에서 사용할 스폰/기준 위치
	virtual FVector GetHomeLocation() const = 0;
	virtual void ApplyCombatMovePresentationContext(bool bMaintainTargetFocus, bool bUseCombatAimOffset) = 0;
	virtual void ClearCombatMovePresentationContext() = 0;
};

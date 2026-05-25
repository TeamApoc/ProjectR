// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/Character/Enemy/PRBossBaseCharacter.h"
#include "PRFaerinCharacter.generated.h"

class UPRFaerinDebugDrawComponent;
class UPRFaerinCombatDirectorComponent;
class UMotionWarpingComponent;

// Faerin 보스 본체 클래스다.
// 패턴 분기와 포털/검 생성 로직은 넣지 않고, 보스 공통 베이스에 Faerin 기본 데이터만 얹는다.
UCLASS(Blueprintable)
class PROJECTR_API APRFaerinCharacter : public APRBossBaseCharacter
{
	GENERATED_BODY()

public:
	APRFaerinCharacter();

protected:
	/*~ AActor Interface ~*/
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	// Faerin PIE 패턴/감지 범위를 표시하는 디버그 컴포넌트다.
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|Debug")
	TObjectPtr<UPRFaerinDebugDrawComponent> DebugDrawComponent;

	// Faerin 원작형 공격 루프를 실행하는 전투 디렉터 컴포넌트다.
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|AI|Boss|Faerin")
	TObjectPtr<UPRFaerinCombatDirectorComponent> CombatDirectorComponent;

	// 공격 루트모션 방향 보정을 위한 Motion Warping 컴포넌트다.
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|Animation")
	TObjectPtr<UMotionWarpingComponent> MotionWarpingComponent;

private:
	// EventManager로 보스 조우 시작 알림 브로드캐스트
	void BroadcastBossEncounterBegin();

	// EventManager로 보스 조우 종료 알림 브로드캐스트
	void BroadcastBossEncounterEnd();
};

// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/Character/Enemy/PRBossBaseCharacter.h"
#include "PRFaerinCharacter.generated.h"

class UPRFaerinDebugDrawComponent;
class UPRFaerinCombatDirectorComponent;
class UPRFaerinGodFallComponent;
class UMotionWarpingComponent;
class UNiagaraSystem;

// Faerin 보스 본체 클래스다.
// 패턴 분기와 포털/검 생성 로직은 넣지 않고, 보스 공통 베이스에 Faerin 기본 데이터만 얹는다.
UCLASS(Blueprintable)
class PROJECTR_API APRFaerinCharacter : public APRBossBaseCharacter
{
	GENERATED_BODY()

public:
	APRFaerinCharacter();

	// God Fall 맵 배치 Rig 전환과 지속 검 hazard를 담당하는 component를 반환한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|AI|Boss|Faerin|GodFall")
	UPRFaerinGodFallComponent* GetGodFallComponent() const { return GodFallComponent; }

	// 근거리 텔레포트 순간 보스 몸 위치의 Niagara를 모든 클라이언트에 재생한다.
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SpawnNearTeleportBodyNiagara(UNiagaraSystem* NiagaraSystem, FName AttachSocketName);

	// 근거리 텔레포트 사라짐/재등장 상태를 모든 클라이언트에 맞춘다.
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SetNearTeleportHidden(bool bShouldHide);

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

	// Phase2 God Fall 진입과 StaticSword hazard 생명주기를 관리하는 컴포넌트다.
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|AI|Boss|Faerin|GodFall")
	TObjectPtr<UPRFaerinGodFallComponent> GodFallComponent;

	// 공격 루트모션 방향 보정을 위한 Motion Warping 컴포넌트다.
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|Animation")
	TObjectPtr<UMotionWarpingComponent> MotionWarpingComponent;

private:
	// EventManager로 보스 조우 시작 알림 브로드캐스트
	void BroadcastBossEncounterBegin();

	// EventManager로 보스 조우 종료 알림 브로드캐스트
	void BroadcastBossEncounterEnd();
};

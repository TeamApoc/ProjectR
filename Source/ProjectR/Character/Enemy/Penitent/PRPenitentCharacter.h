// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/Character/Enemy/PREnemyBaseCharacter.h"
#include "PRPenitentCharacter.generated.h"

class UMotionWarpingComponent;
class UPRSoldierArmoredDebugDrawComponent;
class APRGroundBoxProjectileBase;
class USpringArmComponent;

UCLASS()
class PROJECTR_API APRPenitentCharacter : public APREnemyBaseCharacter
{
	GENERATED_BODY()

public:
	// Penitent 캐릭터 기본값을 초기화한다.
	APRPenitentCharacter();

	// 현재 소환된 배리어 액터를 반환한다.
	APRGroundBoxProjectileBase* GetSpawnedBarrierActor() const { return SpawnedBarrierActor.Get(); }

	// 배리어 액터 부착 씬 컴포넌트를 반환한다.
	USceneComponent* GetBarrierAttachPoint() const {return BarrierAttachPoint.Get();}
	
	// 현재 소환된 배리어 액터 참조를 저장한다.
	void SetSpawnedBarrierActor(APRGroundBoxProjectileBase* InBarrierActor);

	// 현재 소환된 배리어 액터 참조를 제거한다.
	void ClearSpawnedBarrierActor(APRGroundBoxProjectileBase* BarrierActorToClear = nullptr);

	// 소환 배리어 상태 및 참조 정리
	void CleanupSummonedBarrier(bool bRequestBarrierEnd);

	// 현재 유효한 배리어 보유 여부를 반환한다.
	bool HasActiveBarrier() const;

	// 소환 배리어 소멸 처리
	UFUNCTION()
	void HandleSpawnedBarrierDestroyed(APRGroundBoxProjectileBase* DestroyedBarrier);

protected:
	/*~ APREnemyBaseCharacter Interface ~*/
	// 사망 상태 변경 처리
	virtual void HandleDeadTagChanged(bool bEntered) override;

	// 현재 소환되어 유지 중인 배리어 액터
	UPROPERTY(VisibleInstanceOnly, Category = "ProjectR|Penitent")
	TObjectPtr<APRGroundBoxProjectileBase> SpawnedBarrierActor;
	
	// 배리어가 부착되는 씬 컴포넌트
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Penitent")
	TObjectPtr<USceneComponent> BarrierAttachPoint;
	
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Penitent")
	TObjectPtr<USpringArmComponent> BarrierBoom;
	
	// Soldier_Armored PIE debug range draw
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|Debug")
	TObjectPtr<UPRSoldierArmoredDebugDrawComponent> DebugDrawComponent;
	
	// 공격 루트모션 방향 보정용 Motion Warping
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|Animation")
	TObjectPtr<UMotionWarpingComponent> MotionWarpingComponent;
};

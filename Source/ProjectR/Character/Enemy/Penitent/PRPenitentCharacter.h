// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/Character/Enemy/PREnemyBaseCharacter.h"
#include "PRPenitentCharacter.generated.h"

class APRPenitentBarrierActor;

UCLASS()
class PROJECTR_API APRPenitentCharacter : public APREnemyBaseCharacter
{
	GENERATED_BODY()

public:
	// Penitent 캐릭터 기본값을 초기화한다.
	APRPenitentCharacter();

	// 현재 소환된 배리어 액터를 반환한다.
	APRPenitentBarrierActor* GetSpawnedBarrierActor() const { return SpawnedBarrierActor.Get(); }

	// 배리어 액터 부착 씬 컴포넌트를 반환한다.
	USceneComponent* GetBarrierAttachPoint() const {return BarrierAttachPoint.Get();}
	
	// 현재 소환된 배리어 액터 참조를 저장한다.
	void SetSpawnedBarrierActor(APRPenitentBarrierActor* InBarrierActor);

	// 현재 소환된 배리어 액터 참조를 제거한다.
	void ClearSpawnedBarrierActor(APRPenitentBarrierActor* BarrierActorToClear = nullptr);

	// 현재 유효한 배리어 보유 여부를 반환한다.
	bool HasActiveBarrier() const;
	

protected:
	// 현재 소환되어 유지 중인 배리어 액터
	UPROPERTY(VisibleInstanceOnly, Category = "ProjectR|Penitent")
	TObjectPtr<APRPenitentBarrierActor> SpawnedBarrierActor;
	
	// 배리어가 부착되는 씬 컴포넌트
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Penitent")
	TObjectPtr<USceneComponent> BarrierAttachPoint;
};

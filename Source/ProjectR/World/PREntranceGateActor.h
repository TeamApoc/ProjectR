// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PRInteractableActor.h"
#include "PREntranceGateActor.generated.h"

class UBoxComponent;

UCLASS()
class PROJECTR_API APREntranceGateActor : public APRInteractableActor
{
	GENERATED_BODY()

public:
	// 입장 게이트 상호작용 액터 기본 구성
	APREntranceGateActor();

protected:
	// 플레이어 상호작용 감지용 콜리전
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|EntranceGate")
	TObjectPtr<UBoxComponent> InteractionCollision;
};

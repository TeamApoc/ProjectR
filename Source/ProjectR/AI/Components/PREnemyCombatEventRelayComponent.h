// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PREnemyCombatEventRelayComponent.generated.h"

// 이전에는 상태 태그 변화를 GameplayEvent로 중계하던 컴포넌트다.
// 현재는 AttributeSet/BossBase가 서버에서 직접 이벤트를 발행하므로, 기존 BP 참조 보호용으로만 남겨둔다.
UCLASS(ClassGroup = (ProjectR), meta = (BlueprintSpawnableComponent))
class PROJECTR_API UPREnemyCombatEventRelayComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPREnemyCombatEventRelayComponent();

	virtual void BeginPlay() override;

protected:
	// 기존 에셋 호환을 위해 BeginPlay 실행 여부만 기록한다.
	bool bEventsBound = false;
};

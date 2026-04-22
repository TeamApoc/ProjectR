// Copyright ProjectR. All Rights Reserved.

#include "PREnemyCombatEventRelayComponent.h"

UPREnemyCombatEventRelayComponent::UPREnemyCombatEventRelayComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}

void UPREnemyCombatEventRelayComponent::BeginPlay()
{
	Super::BeginPlay();

	// 상태 이벤트는 AttributeSet/BossBase에서 직접 발행한다.
	// 이 컴포넌트는 기존 BP 서브오브젝트 참조가 깨지지 않도록 남긴 호환 레이어다.
	bEventsBound = true;
}
